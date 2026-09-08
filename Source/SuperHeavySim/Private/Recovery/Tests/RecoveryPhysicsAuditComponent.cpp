#include "Recovery/Tests/RecoveryPhysicsAuditComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Chaos/SimCallbackObject.h"
#include "PBDRigidsSolver.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "HAL/IConsoleManager.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Serialization/JsonSerializer.h"

struct FRecoveryCadenceOutput : Chaos::FSimCallbackOutput
{
    double DeltaS=0;
    void Reset() { DeltaS=0; }
};

class FRecoveryCadenceCallback : public Chaos::TSimCallbackObject<Chaos::FSimCallbackNoInput,FRecoveryCadenceOutput>
{
    virtual void OnPreSimulate_Internal() override
    {
        GetProducerOutputData_Internal().DeltaS=GetDeltaTime_Internal();
    }
};

URecoveryPhysicsAuditComponent::URecoveryPhysicsAuditComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}

void URecoveryPhysicsAuditComponent::BeginPlay()
{
    Super::BeginPlay();
    bEnabled=FParse::Param(FCommandLine::Get(),TEXT("RecoveryPhysicsAudit"));
    bJitterClock=bEnabled && FApp::UseFixedTimeStep() && FParse::Param(FCommandLine::Get(),TEXT("RecoveryJitterClock"));
    OriginalFixedDeltaS=FApp::GetFixedDeltaTime();
    if(bJitterClock)DisplayClockHandle=FCoreDelegates::OnBeginFrame.AddUObject(this,&URecoveryPhysicsAuditComponent::AdvanceDisplayClock);
    SetComponentTickEnabled(bEnabled);
    if(bEnabled)
        if(auto* Scene=GetWorld()->GetPhysicsScene())
            Callback=Scene->GetSolver()->CreateAndRegisterSimCallbackObject_External<FRecoveryCadenceCallback>();
}

void URecoveryPhysicsAuditComponent::ConsumePhysicsSteps()
{
    if(Callback)
        while(auto Output=Callback->PopOutputData_External())PhysicsSteps.Add(Output->DeltaS);
}

void URecoveryPhysicsAuditComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    ConsumePhysicsSteps();
}

void URecoveryPhysicsAuditComponent::AdvanceDisplayClock()
{
    // Run before the engine updates its clock. Component tasks inherit a time
    // context in UE 5.8; changing that context did not change subsequent frames.
    constexpr double Pattern[]={1./144,1./59,1./30,1./144,.2,1./60};
    FApp::SetFixedDeltaTime(Pattern[FramePatternIndex++%UE_ARRAY_COUNT(Pattern)]);
}

void URecoveryPhysicsAuditComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bJitterClock)
    {
        FCoreDelegates::OnBeginFrame.Remove(DisplayClockHandle);
        FApp::SetFixedDeltaTime(OriginalFixedDeltaS);
        // Restore the root engine time context too, if another world follows.
        const auto Restore=MakeShared<FDelegateHandle>();
        *Restore=FCoreDelegates::OnBeginFrame.AddLambda([Restore,Delta=OriginalFixedDeltaS]()
        {FApp::SetFixedDeltaTime(Delta);FCoreDelegates::OnBeginFrame.Remove(*Restore);});
    }
    if(bEnabled)
    {
        ConsumePhysicsSteps();
        auto Report=MakeShared<FJsonObject>();
        const auto Stats=[](const FRecoveryStepStatistics& Steps)
        {
            auto Result=MakeShared<FJsonObject>();
            Result->SetNumberField(TEXT("count"),Steps.Count);
            Result->SetNumberField(TEXT("total_s"),Steps.TotalS);
            Result->SetNumberField(TEXT("min_s"),Steps.Count?Steps.MinimumS:0.);
            Result->SetNumberField(TEXT("max_s"),Steps.MaximumS);
            Result->SetNumberField(TEXT("mean_s"),Steps.Count?Steps.TotalS/Steps.Count:0.);
            return Result;
        };
        Report->SetNumberField(TEXT("schema_version"),4);
        Report->SetBoolField(TEXT("measured"),PhysicsSteps.Count>0 && GameSteps.Count>0);
        Report->SetObjectField(TEXT("solver_steps"),Stats(PhysicsSteps));
        Report->SetObjectField(TEXT("game_steps"),Stats(GameSteps));
        if(const auto* Director=Cast<ASuperHeavyRecoveryDirector>(GetOwner()))
        {
            const auto& State=Director->GetDynamicsState();
            FRecoveryStepStatistics Dynamics;
            Dynamics.Count=State.Steps;Dynamics.TotalS=State.ElapsedS;
            Dynamics.MinimumS=State.MinimumStepS;Dynamics.MaximumS=State.MaximumStepS;
            Report->SetObjectField(TEXT("inner_dynamics_steps"),Stats(Dynamics));
            const auto& Guidance=Director->GetGuidanceState();
            FRecoveryStepStatistics Flight;
            Flight.Count=Guidance.Steps;Flight.TotalS=Guidance.ElapsedS;
            Flight.MinimumS=Guidance.MinimumStepS;Flight.MaximumS=Guidance.MaximumStepS;
            Report->SetObjectField(TEXT("flight_guidance_steps"),Stats(Flight));
            const auto& Stage=Director->GetUpperStageState();
            FRecoveryStepStatistics Upper;
            Upper.Count=Stage.Steps;Upper.TotalS=Stage.ElapsedS;
            Upper.MinimumS=Stage.MinimumStepS;Upper.MaximumS=Stage.MaximumStepS;
            Report->SetObjectField(TEXT("upper_stage_steps"),Stats(Upper));
            Report->SetNumberField(TEXT("mission_generation"),Director->GetMissionGeneration());
        }
        Report->SetStringField(TEXT("scope"),TEXT("Session cadence, including ground preparation; proof of scheduling only, not flight convergence."));
        const auto* Settings=UPhysicsSettings::Get();
        Report->SetBoolField(TEXT("substepping"),Settings->bSubstepping);
        Report->SetBoolField(TEXT("async_physics"),Settings->bTickPhysicsAsync);
        Report->SetBoolField(TEXT("jitter_clock"),bJitterClock);
        Report->SetNumberField(TEXT("configured_frame_cap_s"),Settings->MaxPhysicsDeltaTime);
        if(const auto* Block=IConsoleManager::Get().FindConsoleVariable(TEXT("p.AsyncPhysicsBlockMode")))
            Report->SetNumberField(TEXT("async_block_mode"),Block->GetInt());
        Report->SetNumberField(TEXT("configured_max_substep_s"),Settings->MaxSubstepDeltaTime);
        if(auto* Scene=GetWorld()->GetPhysicsScene())
            Report->SetNumberField(TEXT("configured_fixed_step_s"),Scene->GetSolver()->GetAsyncDeltaTime());
        FString Name=TEXT("Physics");FParse::Value(FCommandLine::Get(),TEXT("RecoveryReportName="),Name);
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Recovery");
        IFileManager::Get().MakeDirectory(*Directory,true);
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(Directory/(FPaths::MakeValidFileName(Name)+TEXT("-cadence.json"))));
    }
    if(Callback)
    {
        if(auto* Scene=GetWorld()->GetPhysicsScene())Scene->GetSolver()->UnregisterAndFreeSimCallbackObject_External(Callback);
        Callback=nullptr;
    }
    Super::EndPlay(Reason);
}

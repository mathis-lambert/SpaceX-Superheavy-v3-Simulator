#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Chaos/SimCallbackObject.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "PBDRigidsSolver.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

struct FRecoveryDynamicsSetup
{
    FRecoveryGuidanceConfiguration Configuration;
    TArray<FRecoveryEngineState> Geometry;
    double FuelKg=0,RcsFuelKg=0;
    uint32 Generation=0;
};
struct FRecoveryDynamicsInput : Chaos::FSimCallbackInput
{
    TSharedPtr<const FRecoveryDynamicsSetup,ESPMode::ThreadSafe> Setup;
    Chaos::FSingleParticlePhysicsProxy* Proxy=nullptr;
    FRecoveryDynamicsCommand Command;
    FVector WorldGravityMps2=FVector::ZeroVector;
    void Reset() { Setup.Reset();Proxy=nullptr; }
};
struct FRecoveryDynamicsOutput : Chaos::FSimCallbackOutput
{
    FRecoveryDynamicsState State;
    FRecoveryGuidanceState Guidance;
    uint32 Generation=0;
    void Reset() { Generation=0; } // Array storage is reused by the output pool.
};

class FRecoveryPhysicsCallback : public Chaos::TSimCallbackObject<FRecoveryDynamicsInput,FRecoveryDynamicsOutput>
{
    FRecoveryDynamicsModel Model;
    FRecoveryGuidanceModel Guidance;
    TSharedPtr<const FRecoveryDynamicsSetup,ESPMode::ThreadSafe> ActiveSetup;
    virtual void OnPreSimulate_Internal() override
    {
        const auto* Input=GetConsumerInput_Internal();
        if(!Input || !Input->Setup || !Input->Proxy)return;
        auto* Handle=Input->Proxy->GetPhysicsThreadAPI();
        if(!Handle || (Handle->ObjectState()!=Chaos::EObjectStateType::Dynamic && Handle->ObjectState()!=Chaos::EObjectStateType::Sleeping))return;
        if(ActiveSetup!=Input->Setup)
        {
            ActiveSetup=Input->Setup;
            Model.Reset(ActiveSetup->Configuration,ActiveSetup->Geometry,ActiveSetup->FuelKg,ActiveSetup->RcsFuelKg);
            Guidance.Reset(ActiveSetup->Configuration);
        }
        FRecoveryBodyKinematics Body;
        Body.OriginM=Handle->X()/100.;Body.Rotation=Handle->R();
        Body.VelocityMps=Handle->V()/100.;Body.AngularVelocityWorldRadS=Handle->W();
        Guidance.Step(Body,Model.GetState(),Input->Command,GetDeltaTime_Internal());
        Model.Step(Body,Guidance.GetState().Command,GetDeltaTime_Internal());
        const auto& State=Model.GetState();
        const auto& Mass=State.Mass;
        Handle->SetM(Mass.MassKg);Handle->SetInvM(1./Mass.MassKg);
        const FVector3f Inertia(Mass.InertiaKgM2*10000.);
        Handle->SetI(Inertia);Handle->SetInvI(FVector3f(1.f/Inertia.X,1.f/Inertia.Y,1.f/Inertia.Z));
        Handle->SetCenterOfMass(FVector(0,0,(Mass.CentreFromBaseM-FlightGeometry::BoosterBaseOffsetM)*100.));
        Handle->SetRotationOfMass(FQuat::Identity);
        const FVector COM=Body.OriginM+Body.Rotation.GetUpVector()*(Mass.CentreFromBaseM-FlightGeometry::BoosterBaseOffsetM);
        FVector ForceN=FVector::ZeroVector,MomentNm=FVector::ZeroVector;
        for(const auto& Force:State.Forces)
        {
            ForceN+=Force.ForceN;
            MomentNm+=FVector::CrossProduct(Force.PointCm/100.-COM,Force.ForceN);
        }
        // Gravity is reported as a total load; Chaos also adds world gravity.
        // The only applied moments are sums of physical force/lever-arm products.
        Handle->AddForce((ForceN-Mass.MassKg*Input->WorldGravityMps2)*100.);
        Handle->AddTorque(MomentNm*10000.);
        auto& Output=GetProducerOutputData_Internal();
        Output.State=State;Output.Guidance=Guidance.GetState();Output.Generation=ActiveSetup->Generation;
    }
};

URecoveryPhysicsComponent::URecoveryPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void URecoveryPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    if(auto* Scene=GetWorld()->GetPhysicsScene())
        Callback=Scene->GetSolver()->CreateAndRegisterSimCallbackObject_External<FRecoveryPhysicsCallback>();
}
void URecoveryPhysicsComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Callback)
    {
        if(auto* Scene=GetWorld()->GetPhysicsScene())Scene->GetSolver()->UnregisterAndFreeSimCallbackObject_External(Callback);
        Callback=nullptr;
    }
    Setup.Reset();
    Super::EndPlay(Reason);
}
void URecoveryPhysicsComponent::InitializeMission(const FRecoveryGuidanceConfiguration& Configuration,
    const TArray<FRecoveryEngineState>& Geometry,double FuelKg,double RcsFuelKg,uint32 Generation)
{
    auto Next=MakeShared<FRecoveryDynamicsSetup,ESPMode::ThreadSafe>();
    Next->Configuration=Configuration;Next->Geometry=Geometry;
    Next->FuelKg=FuelKg;Next->RcsFuelKg=RcsFuelKg;Next->Generation=Generation;
    Setup=Next;
}
void URecoveryPhysicsComponent::Submit(UPrimitiveComponent& Body,const FRecoveryDynamicsCommand& Command)
{
    if(!Callback || !Setup)return;
    auto* Input=Callback->GetProducerInputData_External();
    Input->Setup=Setup;Input->Command=Command;
    const auto* Instance=Body.GetBodyInstance();
    Input->Proxy=Instance?Instance->ActorHandle:nullptr;
    Input->WorldGravityMps2=FVector(0,0,GetWorld()->GetGravityZ()/100.);
}
bool URecoveryPhysicsComponent::Consume(FRecoveryDynamicsState& State,FRecoveryGuidanceState& Guidance)
{
    bool Updated=false;
    if(Callback && Setup)
        while(auto Output=Callback->PopOutputData_External())
            if(Output->Generation==Setup->Generation){State=Output->State;Guidance=Output->Guidance;Updated=true;}
    return Updated;
}
void URecoveryPhysicsComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(auto* Director=Cast<ASuperHeavyRecoveryDirector>(GetOwner()))Director->ConsumeDynamicsState();
}

FRecoveryDynamicsConfiguration URecoveryPhysicsComponent::BuildConfiguration(const USuperHeavyRecoveryProfile& P)
{
    FRecoveryDynamicsConfiguration C;
    C.Engines.MinimumThrottle=P.MinimumThrottle;C.Engines.OpeningTimeConstantS=P.ThrottleTimeConstant;
    C.Engines.ShutdownTimeS=P.EngineShutdownTimeS;C.Engines.MaximumGimbalDeg=P.MaxGimbalDeg;C.Engines.GimbalRateDegS=P.GimbalRateDegS;
    C.DryMassKg=P.DryMassKg;C.UpperStageMassKg=P.UpperStageMassKg;C.MixtureRatio=P.MixtureRatio;
    C.OxygenDensityKgM3=P.OxygenDensityKgM3;C.MethaneDensityKgM3=P.MethaneDensityKgM3;
    C.OxygenTankBottomM=P.OxygenTankBottomM;C.MethaneTankBottomM=P.MethaneTankBottomM;
    C.EngineThrustN=P.EngineThrustN;C.SpecificImpulseSeaLevelS=P.SpecificImpulseSeaLevelS;C.SpecificImpulseVacuumS=P.SpecificImpulseVacuumS;
    C.ReactionControlTorqueNm=P.ReactionControlTorqueNm;C.ReactionValveTimeConstantS=P.ReactionValveTimeConstantS;
    C.DragAreaM2=P.DragAreaM2;C.AxialDragCoefficient=P.AxialDragCoefficient;C.TailFirstDragCoefficient=P.TailFirstDragCoefficient;
    C.BodySideAreaM2=P.BodySideAreaM2;C.BodyNormalCoefficient=P.BodyNormalCoefficient;C.GridFinDragCoefficient=P.GridFinDragCoefficient;
    C.GridFinAreaM2=P.GridFinAreaM2;C.GridFinLiftSlope=P.GridFinLiftSlope;C.GridFinMaxAngleDeg=P.GridFinMaxAngleDeg;C.GridFinRateDegS=P.GridFinRateDegS;
    C.SeaLevelTemperatureOffsetK=P.SeaLevelTemperatureOffsetK;C.WindVelocityMps=P.WindVelocityMps;
    C.ConditioningVentKgS=P.ConditioningVentKgS;C.ConditioningJetSpeedMps=P.ConditioningJetSpeedMps;
    return C;
}

FRecoveryGuidanceConfiguration URecoveryPhysicsComponent::BuildGuidanceConfiguration(const USuperHeavyRecoveryProfile& P)
{
    FRecoveryGuidanceConfiguration Configuration;
    static_cast<FRecoveryDynamicsConfiguration&>(Configuration)=URecoveryPhysicsComponent::BuildConfiguration(P);
    Configuration.ApogeeM=P.ApogeeM;
    Configuration.AscentDurationS=P.AscentDurationS;
    Configuration.AscentPitchDeg=P.AscentPitchDeg;
    Configuration.AscentMaxAccelerationMps2=P.AscentMaxAccelerationMps2;
    Configuration.LandingReserveKg=P.LandingReserveKg;
    Configuration.BoostbackReserveKg=P.BoostbackReserveKg;
    Configuration.LandingDriftCorrectionS=P.LandingDriftCorrectionS;
    Configuration.LandingIgnitionCeilingM=P.LandingIgnitionCeilingM;
    Configuration.LandingBurnMarginM=P.LandingBurnMarginM;
    Configuration.LandingDecelerationMps2=P.LandingDecelerationMps2;
    Configuration.MaxEntryAngleDeg=P.MaxEntryAngleDeg;
    Configuration.MaxTiltDeg=P.MaxTiltDeg;
    Configuration.TimeoutSeconds=P.TimeoutSeconds;
    Configuration.CatchLugPlusM=P.CatchLugPlusM;
    Configuration.CatchLugMinusM=P.CatchLugMinusM;
    Configuration.CaptureHeadingDeg=P.CaptureHeadingDeg;
    return Configuration;
}

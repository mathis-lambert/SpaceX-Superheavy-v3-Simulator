#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoverySeparationModel.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Chaos/SimCallbackObject.h"
#include "Chaos/Collision/CollisionConstraintAllocator.h"
#include "Chaos/Collision/PBDCollisionConstraint.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "PBDRigidsSolver.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

struct FRecoveryDynamicsSetup
{
    FRecoveryGuidanceConfiguration Configuration;
    FRecoveryUpperStageConfiguration UpperStage;
    TArray<FRecoveryEngineState> Geometry;
    double FuelKg=0,RcsFuelKg=0;
    uint32 Generation=0;
};
struct FRecoveryDynamicsInput : Chaos::FSimCallbackInput
{
    TSharedPtr<const FRecoveryDynamicsSetup,ESPMode::ThreadSafe> Setup;
    Chaos::FSingleParticlePhysicsProxy* Proxy=nullptr;
    Chaos::FSingleParticlePhysicsProxy* UpperStageProxy=nullptr;
    Chaos::FSingleParticlePhysicsProxy* RailProxy[2]={nullptr,nullptr};
    FRecoveryDynamicsCommand Command;
    FVector WorldGravityMps2=FVector::ZeroVector;
    void Reset() { Setup.Reset();Proxy=nullptr;UpperStageProxy=nullptr;RailProxy[0]=RailProxy[1]=nullptr; }
};
struct FRecoveryDynamicsOutput : Chaos::FSimCallbackOutput
{
    FRecoveryDynamicsState State;
    FRecoveryGuidanceState Guidance;
    FRecoveryUpperStageState UpperStage;
    uint32 Generation=0;
    void Reset() { Generation=0; } // Array storage is reused by the output pool.
};

class FRecoveryPhysicsCallback : public Chaos::TSimCallbackObject<FRecoveryDynamicsInput,FRecoveryDynamicsOutput,
    Chaos::ESimCallbackOptions::Presimulate|Chaos::ESimCallbackOptions::PostSolve>
{
    FRecoveryDynamicsModel Model;
    FRecoveryGuidanceModel Guidance;
    FRecoveryUpperStageModel UpperStage;
    FRecoveryRailSupport RailSupport;
    TSharedPtr<const FRecoveryDynamicsSetup,ESPMode::ThreadSafe> ActiveSetup;

    template<typename THandle>
    static FRecoveryBodyKinematics ReadBody(const THandle* Handle)
    {
        FRecoveryBodyKinematics Body;
        Body.OriginM=Handle->X()/100.;Body.Rotation=Handle->R();
        Body.VelocityMps=Handle->V()/100.;Body.AngularVelocityWorldRadS=Handle->W();
        return Body;
    }
    template<typename THandle>
    static void ApplyLoads(THandle* Handle,const FRecoveryBodyKinematics& Body,const RecoveryMass::FProperties& Mass,
        double OriginFromBaseM,TConstArrayView<FRecoveryForceSample> Forces,const FVector& WorldGravityMps2)
    {
        Handle->SetM(Mass.MassKg);Handle->SetInvM(1./Mass.MassKg);
        const FVector3f Inertia(Mass.InertiaKgM2*10000.);
        Handle->SetI(Inertia);Handle->SetInvI(FVector3f(1.f/Inertia.X,1.f/Inertia.Y,1.f/Inertia.Z));
        Handle->SetCenterOfMass(FVector(0,0,(Mass.CentreFromBaseM-OriginFromBaseM)*100.));
        Handle->SetRotationOfMass(FQuat::Identity);
        const FVector COM=Body.OriginM+Body.Rotation.GetUpVector()*(Mass.CentreFromBaseM-OriginFromBaseM);
        FVector ForceN=FVector::ZeroVector,MomentNm=FVector::ZeroVector;
        for(const auto& Force:Forces)
        {
            ForceN+=Force.ForceN;
            MomentNm+=FVector::CrossProduct(Force.PointCm/100.-COM,Force.ForceN);
        }
        // Chaos supplies world gravity; the models report the complete load.
        Handle->AddForce((ForceN-Mass.MassKg*WorldGravityMps2)*100.);
        Handle->AddTorque(MomentNm*10000.);
    }
    virtual void OnPreSimulate_Internal() override
    {
        if(GetDeltaTime_Internal()<=0)return; // Paused/flush callbacks apply no cached load.
        const auto* Input=GetConsumerInput_Internal();
        if(!Input || !Input->Setup || !Input->Proxy)return;
        auto* Handle=Input->Proxy->GetPhysicsThreadAPI();
        if(!Handle || (Handle->ObjectState()!=Chaos::EObjectStateType::Dynamic && Handle->ObjectState()!=Chaos::EObjectStateType::Sleeping))return;
        if(ActiveSetup!=Input->Setup)
        {
            ActiveSetup=Input->Setup;
            Model.Reset(ActiveSetup->Configuration,ActiveSetup->Geometry,ActiveSetup->FuelKg,ActiveSetup->RcsFuelKg);
            Guidance.Reset(ActiveSetup->Configuration);
            UpperStage.Reset(ActiveSetup->UpperStage);
            RailSupport=FRecoveryRailSupport();
        }
        auto Body=ReadBody(Handle);
        auto Command=Input->Command;
        // Previous physical step's resolved support, independent of hit-event
        // delivery and render-frame duration. This observation adds no force.
        Command.SupportContactCount=RailSupport.Count();
        auto* StageHandle=Input->UpperStageProxy?Input->UpperStageProxy->GetPhysicsThreadAPI():nullptr;
        if(Command.bSeparated && StageHandle && !UpperStage.GetState().bSeparated)
        {
            const auto& Before=Model.GetState();
            const auto BoosterMass=RecoveryMass::Booster(ActiveSetup->Configuration,Before.PropellantKg,Before.RcsPropellantKg,false);
            const auto Split=RecoverySeparation::Split(Body,Before.Mass,BoosterMass,UpperStage.GetState().Mass);
            // Atomic mechanical separation: both initial states use this solver
            // sample. Render interpolation never participates in the transfer.
            StageHandle->SetX(Split.UpperStage.OriginM*100.);StageHandle->SetR(Split.UpperStage.Rotation);
            StageHandle->SetV(Split.UpperStage.VelocityMps*100.);StageHandle->SetW(Split.UpperStage.AngularVelocityWorldRadS);
            Handle->SetV(Split.Booster.VelocityMps*100.);Body=Split.Booster;
            UpperStage.RecordSeparation(BoosterMass.MassKg,(StageHandle->V()/100.-Split.UpperStage.VelocityMps).Size(),
                Split.LinearMomentumRelativeError,Split.AngularMomentumRelativeError);
        }
        // The guide acknowledges physical separation, not merely proxy creation.
        Command.bSeparated=UpperStage.GetState().bSeparated;
        if(Input->Command.bExternalFlightFixture)Command.bSeparated=Input->Command.bSeparated;
        Guidance.Step(Body,Model.GetState(),Command,GetDeltaTime_Internal());
        Model.Step(Body,Guidance.GetState().Command,GetDeltaTime_Internal());
        const auto& State=Model.GetState();
        ApplyLoads(Handle,Body,State.Mass,FlightGeometry::BoosterBaseOffsetM,State.Forces,Input->WorldGravityMps2);
        if(UpperStage.GetState().bSeparated)
            if(StageHandle &&
                (StageHandle->ObjectState()==Chaos::EObjectStateType::Dynamic || StageHandle->ObjectState()==Chaos::EObjectStateType::Sleeping))
            {
                const auto StageBody=ReadBody(StageHandle);
                UpperStage.Step(StageBody,Input->Command.Experiment.WindScale,GetDeltaTime_Internal());
                const auto& Stage=UpperStage.GetState();
                ApplyLoads(StageHandle,StageBody,Stage.Mass,FlightGeometry::UpperStageCentreFromBaseM,Stage.Forces,Input->WorldGravityMps2);
            }
        auto& Output=GetProducerOutputData_Internal();
        Output.State=State;Output.Guidance=Guidance.GetState();Output.UpperStage=UpperStage.GetState();Output.Generation=ActiveSetup->Generation;
        Output.State.RailSupport=RailSupport;
    }

    virtual void OnPostSolve_Internal() override
    {
        const auto* Input=GetConsumerInput_Internal();
        if(GetDeltaTime_Internal()<=0 || !ActiveSetup || !Input || Input->Setup!=ActiveSetup || !Input->Proxy)return;
        const auto* Particle=Input->Proxy->GetHandle_LowLevel();
        if(!Particle)return;
        RailSupport.CurrentMask=0;++RailSupport.SolverSamples;
        FVector StepImpulseNs[2]={FVector::ZeroVector,FVector::ZeroVector};
        Particle->ParticleCollisions().VisitConstCollisions([&](const Chaos::FPBDCollisionConstraint& Contact)
        {
            using Result=Chaos::ECollisionVisitorResult;
            if(Contact.GetIsProbe() || (Contact.GetDisabled() && !Contact.IsSleeping()))return Result::Continue;
            const int32 BodyIndex=Contact.GetParticle0()==Particle?0:1;
            const auto* Other=Contact.GetParticle(1-BodyIndex);
            int32 Side=-1;
            for(int32 I=0;I<2;++I)
                if(Input->RailProxy[I] && Input->RailProxy[I]->GetHandle_LowLevel()==Other)Side=I;
            if(Side<0)return Result::Continue;
            for(int32 I=0;I<Contact.NumManifoldPoints();++I)
            {
                const auto& Point=Contact.GetManifoldPoint(I);
                const auto& Solved=Contact.GetManifoldPointResult(I);
                if(Point.Flags.bDisabled || !Solved.bIsValid ||
                    (Solved.NetImpulse.IsNearlyZero() && Solved.NetPushOut.IsNearlyZero()))continue;
                const FVector Normal=Contact.GetShapeWorldTransform1().GetRotation().RotateVector(
                    FVector(Point.ContactPoint.ShapeContactNormal))*(BodyIndex==0?1.:-1.);
                if(Normal.Z<=0.4)continue;
                const FVector Local=Contact.GetShapeRelativeTransform(BodyIndex).TransformPosition(
                    FVector(Point.ContactPoint.ShapeContactPoints[BodyIndex]))/100.+FVector(0,0,FlightGeometry::BoosterBaseOffsetM);
                const auto& Config=ActiveSetup->Configuration;
                if(!RecoveryContactGeometry::AtFitting(Local-Config.CatchLugPlusM) &&
                    !RecoveryContactGeometry::AtFitting(Local-Config.CatchLugMinusM))continue;
                RailSupport.CurrentMask|=1u<<Side;RailSupport.EverMask|=1u<<Side;
                // A sleeping manifold retains its solved values. Do not add the
                // same historical impulse a second time while it sleeps.
                if(!Contact.IsSleeping())StepImpulseNs[Side]+=(FVector(Solved.NetImpulse)+
                    FVector(Solved.NetPushOut)/GetDeltaTime_Internal())*((BodyIndex==0?1.:-1.)/100.);
            }
            return Result::Continue;
        });
        // Chaos reports position-solver reaction as NetPushOut / Dt alongside
        // velocity impulse. Sum each rail's vectors before accumulating size.
        for(int32 Side=0;Side<2;++Side)
        {
            RailSupport.ImpulseNs[Side]+=StepImpulseNs[Side].Size();
            RailSupport.ReactionImpulseWorldNs[Side]+=StepImpulseNs[Side];
        }
        GetProducerOutputData_Internal().State.RailSupport=RailSupport;
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
    {
        // Explicit numerical-convergence runs may refine the physical clock.
        // This changes solver scheduling, never a body's state or forces.
        double PhysicsHz=0;
        if(FParse::Value(FCommandLine::Get(),TEXT("RecoveryPhysicsHz="),PhysicsHz) &&
            FMath::IsFinite(PhysicsHz) && PhysicsHz>=60 && PhysicsHz<=960)
            Scene->GetSolver()->SetAsyncDeltaTime(1./PhysicsHz);
        Callback=Scene->GetSolver()->CreateAndRegisterSimCallbackObject_External<FRecoveryPhysicsCallback>();
    }
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
    const TArray<FRecoveryEngineState>& Geometry,double FuelKg,double RcsFuelKg,const FRecoveryUpperStageConfiguration& UpperStage,uint32 Generation)
{
    auto Next=MakeShared<FRecoveryDynamicsSetup,ESPMode::ThreadSafe>();
    Next->Configuration=Configuration;Next->Geometry=Geometry;
    Next->UpperStage=UpperStage;
    Next->FuelKg=FuelKg;Next->RcsFuelKg=RcsFuelKg;Next->Generation=Generation;
    Setup=Next;
}
void URecoveryPhysicsComponent::Submit(UPrimitiveComponent& Body,UPrimitiveComponent* UpperStage,
    UPrimitiveComponent& LeftRail,UPrimitiveComponent& RightRail,const FRecoveryDynamicsCommand& Command)
{
    if(!Callback || !Setup)return;
    auto* Input=Callback->GetProducerInputData_External();
    Input->Setup=Setup;Input->Command=Command;
    const auto* Instance=Body.GetBodyInstance();
    Input->Proxy=Instance?Instance->ActorHandle:nullptr;
    const auto* UpperInstance=UpperStage?UpperStage->GetBodyInstance():nullptr;
    Input->UpperStageProxy=UpperInstance?UpperInstance->ActorHandle:nullptr;
    const auto* LeftInstance=LeftRail.GetBodyInstance();const auto* RightInstance=RightRail.GetBodyInstance();
    Input->RailProxy[0]=LeftInstance?LeftInstance->ActorHandle:nullptr;
    Input->RailProxy[1]=RightInstance?RightInstance->ActorHandle:nullptr;
    Input->WorldGravityMps2=FVector(0,0,GetWorld()->GetGravityZ()/100.);
}
bool URecoveryPhysicsComponent::Consume(FRecoveryDynamicsState& State,FRecoveryGuidanceState& Guidance,FRecoveryUpperStageState& UpperStage)
{
    bool Updated=false;
    if(Callback && Setup)
        while(auto Output=Callback->PopOutputData_External())
            if(Output->Generation==Setup->Generation){State=Output->State;Guidance=Output->Guidance;UpperStage=Output->UpperStage;Updated=true;}
    return Updated;
}
void URecoveryPhysicsComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(auto* Director=Cast<ASuperHeavyRecoveryDirector>(GetOwner()))Director->ConsumeDynamicsState();
}

FRecoveryUpperStageConfiguration URecoveryPhysicsComponent::BuildUpperStageConfiguration(const USuperHeavyRecoveryProfile& P)
{
    FRecoveryUpperStageConfiguration C;
    C.DryMassKg=P.UpperStageDryMassKg;C.InitialFuelKg=FMath::Max(0.,P.UpperStageMassKg-P.UpperStageDryMassKg);
    C.EngineThrustN=P.UpperStageEngineThrustN;C.SpecificImpulseS=P.UpperStageIspS;
    C.TemperatureOffsetK=P.SeaLevelTemperatureOffsetK;C.SurfaceWindMps=P.WindVelocityMps;
    return C;
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
    Configuration.FrontReturnOffsetM=P.FrontReturnOffsetM;
    Configuration.LandingWindLeadS=P.LandingWindLeadS;
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

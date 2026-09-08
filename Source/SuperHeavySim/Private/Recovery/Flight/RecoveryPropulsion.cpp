#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Vehicle/SuperHeavyVehicleActor.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::InitializePhysicalActuators()
{
    Engines.Reset();
    const FQuat Q=Body->GetComponentQuat();
    const FVector Base=FlightGeometry::BoosterBaseCm(*Body);
    TInlineComponentArray<UChildActorComponent*> EngineChildren(Vehicle);
    for(auto* Child:EngineChildren)
    {
        const FString Id=Child->GetName();
        if(!Id.StartsWith(TEXT("R")) || !Child->GetChildActor()) continue;
        TInlineComponentArray<USceneComponent*> Parts(Child->GetChildActor());
        USceneComponent* Pivot=nullptr;
        for(auto* Part:Parts)if(Part->GetName()==TEXT("GimbalPivot"))Pivot=Part;
        for(auto* Part:Parts) if(Part->GetName()==TEXT("ThrustSocket"))
        {
            FRecoveryEngineState Engine;
            Engine.Id=Child->GetFName();
            const FVector Mount=Pivot?Pivot->GetComponentLocation():Part->GetComponentLocation();
            Engine.PositionFromBaseM=Q.UnrotateVector(Mount-Base)/100.;
            Engine.NozzleOffsetBodyM=Q.UnrotateVector(Part->GetComponentLocation()-Mount)/100.;
            Engine.bGimballed=Id.StartsWith(TEXT("RG"));
            Engine.bCentral=Id.StartsWith(TEXT("RGC"));
            Engines.Add(Engine);
            break;
        }
    }
    ReactionForcesBodyN.Init(FVector::ZeroVector,6);
    UE_LOG(LogTemp,Display,TEXT("PHYSICAL_ENGINE_REGISTRY count=%d"),Engines.Num());
}

void ASuperHeavyRecoveryDirector::ApplyThrust(const FVector& ThrustAcceleration,const FVector& TargetUp,double Dt)
{
    FRecoveryEngineCommand Command;
    Command.RequestedCount=ActiveEngines;
    Command.FailedEngine=Experiment.FailedEngine;
    Command.RatedThrustN=RuntimeProfile->EngineThrustN*EngineIspS/RuntimeProfile->SpecificImpulseSeaLevelS;
    Command.SpecificImpulseS=EngineIspS;
    Command.RequestedThrustN=FMath::Max(0.,FVector::DotProduct(ThrustAcceleration,Body->GetUpVector())*MassKg);
    auto Step=RecoveryPropulsion::AdvanceEngines(Engines,EngineParameters,Command,PropellantKg,Dt);
    ActualThrustN=Dt>0?Step.DeliveredImpulseNs/Dt:0.;
    PropellantKg=FMath::Max(0.,PropellantKg-Step.FuelUsedKg);
    MainFuelConsumedKg+=Step.FuelUsedKg;
    UpdateMass();
    Throttle=Step.AvailableThrustN>0?FMath::Clamp(ActualThrustN/Step.AvailableThrustN,0.,1.):0.;
    const FQuat Q=Body->GetComponentQuat();
    const FVector COM=Q.UnrotateVector(Body->GetCenterOfMass()/100.-BasePositionM);
    const FVector DesiredTorque=Phase<=ERecoveryPhase::Countdown || Phase>=ERecoveryPhase::Captured || bContactShutdown ? FVector::ZeroVector :
        AttitudeTorque(TargetUp,Phase>=ERecoveryPhase::LandingBurn?2.5:.65,Phase>=ERecoveryPhase::LandingBurn?3.2:1.6);
    RecoveryPropulsion::AllocateGimbals(Engines,EngineParameters,COM,DesiredTorque,Dt,Step);
    for(int32 I=0;I<Engines.Num();++I)
    {
        const auto& Engine=Engines[I];
        ApplyVehicleForce(ERecoveryForceKind::Engine,I,Q.RotateVector(Engine.StepForceBodyN),
            (BasePositionM+Q.RotateVector(Engine.PositionFromBaseM))*100);
        const double MeanThrust=Dt>0?Engine.StepImpulseNs/Dt:0.;
        PeakEngineForceRatio=FMath::Max(PeakEngineForceRatio,MeanThrust>1.?Engine.StepForceBodyN.Size()/MeanThrust:0.);
        PeakAppliedGimbalDeg=FMath::Max(PeakAppliedGimbalDeg,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Engine.DirectionBody.Z,-1.,1.))));
    }
    LastEngineForceBodyN=Step.ForceBodyN;
    LastEngineMomentBodyNm=Step.MomentBodyNm;
    AppliedGimbal=FVector(Step.ForceBodyN.X,Step.ForceBodyN.Y,0);
}

void ASuperHeavyRecoveryDirector::ApplyReactionControl(const FVector& TorqueBody,double Dt)
{
    // Each physical pod has two opposing, one-way ports. Signed net force picks
    // the active port; neither port can pull. Valve lag and fuel limit impulse.
    const bool Enabled=!Experiment.bReactionJetsDisabled && ActiveEngines==0 && !bContactShutdown && Phase>=ERecoveryPhase::Ascent && Phase<ERecoveryPhase::Captured;
    const double Blend=1-FMath::Clamp((DynamicPressurePa-100)/1000.,0.,1.);
    const FVector T=Enabled?TorqueBody.GetClampedToMaxSize(RuntimeProfile->ReactionControlTorqueNm*Blend):FVector::ZeroVector;
    const FVector Demand[]={FVector(0,-T.X/50.,0),FVector(0,T.X/50.,0),FVector(T.Y/50.,0,0),FVector(-T.Y/50.,0,0),FVector(-T.Z/9.1,0,0),FVector(T.Z/9.1,0,0)};
    const double Response=1-FMath::Exp(-Dt/FMath::Max(.001,RuntimeProfile->ReactionValveTimeConstantS));
    double Total=0;
    for(int I=0;I<6;++I)
    {
        // A reversing valve first closes its current port; it never cross-fires.
        const FVector Target=FVector::DotProduct(ReactionForcesBodyN[I],Demand[I])<0?FVector::ZeroVector:Demand[I];
        ReactionForcesBodyN[I]=FMath::Lerp(ReactionForcesBodyN[I],Target,Response);
        if(ReactionForcesBodyN[I].Size()<1.)ReactionForcesBodyN[I]=FVector::ZeroVector;
        Total+=ReactionForcesBodyN[I].Size();
    }
    const double Delivered=RecoveryActuators::FuelLimitedThrust(Total,RcsPropellantKg,150.,Dt);
    const double Scale=Total>0?Delivered/Total:0;
    RcsPropellantKg=FMath::Max(0.,RcsPropellantKg-Delivered/(150.*RecoveryAtmosphere::G0)*Dt);
    const FQuat Q=Body->GetComponentQuat();
    RcsTorqueBody=FVector::ZeroVector;
    const auto& Positions=FlightGeometry::ReactionNozzlePositionsM();
    for(int I=0;I<6;++I)
    {
        ReactionForcesBodyN[I]*=Scale;
        const FVector Location=(BasePositionM+Q.RotateVector(Positions[I]))*100;
        ApplyVehicleForce(ERecoveryForceKind::ReactionJet,I,Q.RotateVector(ReactionForcesBodyN[I]),Location);
        RcsTorqueBody+=FVector::CrossProduct(Q.UnrotateVector(Location-Body->GetCenterOfMass())/100.,ReactionForcesBodyN[I]);
    }
}

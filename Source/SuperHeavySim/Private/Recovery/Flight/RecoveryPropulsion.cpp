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
    const double PerEngine=RuntimeProfile->EngineThrustN*EngineIspS/RuntimeProfile->SpecificImpulseSeaLevelS;
    const auto IsRequested=[this](const FRecoveryEngineState& E)
    { return ActiveEngines==33 || (ActiveEngines>=13 && E.bGimballed) || (ActiveEngines>0 && E.bCentral); };
    int32 AvailableCount=0;
    for(int32 I=0;I<Engines.Num();++I)if(IsRequested(Engines[I]) && I!=Experiment.FailedEngine)++AvailableCount;
    const double Available=AvailableCount*PerEngine;
    const double Required=FMath::Max(0.,FVector::DotProduct(ThrustAcceleration,Body->GetUpVector())*MassKg);
    const double Command=Available>0 && PropellantKg>0 ? FMath::Clamp(Required/Available,RuntimeProfile->MinimumThrottle,1.) : 0.;
    const double Response=1-FMath::Exp(-Dt/FMath::Max(.01,RuntimeProfile->ThrottleTimeConstant));
    double Total=0;
    for(auto& Engine:Engines)
    {
        const bool Enabled=IsRequested(Engine) && (&Engine-Engines.GetData())!=Experiment.FailedEngine;
        // A closing propellant valve reaches its seat in finite time. Its rate
        // belongs to the engine model and does not depend on mission phase time.
        Engine.ThrustN=Enabled && Command>0 ? FMath::Lerp(Engine.ThrustN,Command*PerEngine,Response) :
            FMath::Max(0.,Engine.ThrustN-PerEngine*Dt/FMath::Max(.01,RuntimeProfile->EngineShutdownTimeS));
        if(Engine.ThrustN<1.) Engine.ThrustN=0;
        Total+=Engine.ThrustN;
    }
    const double Delivered=RecoveryActuators::FuelLimitedThrust(Total,PropellantKg,EngineIspS,Dt);
    const double FuelScale=Total>0?Delivered/Total:0;
    ActualThrustN=Delivered;
    const double Used=Delivered/(EngineIspS*RecoveryAtmosphere::G0)*Dt;
    PropellantKg=FMath::Max(0.,PropellantKg-Used);MainFuelConsumedKg+=Used;
    UpdateMass();
    Throttle=Available>0?FMath::Clamp(Delivered/Available,0.,1.):0;
    const FQuat Q=Body->GetComponentQuat();
    const FVector COM=Q.UnrotateVector(Body->GetCenterOfMass()/100.-BasePositionM);
    const FVector DesiredTorque=Phase<=ERecoveryPhase::Countdown || Phase>=ERecoveryPhase::Captured || bContactShutdown ? FVector::ZeroVector :
        AttitudeTorque(TargetUp,Phase>=ERecoveryPhase::LandingBurn?2.5:.65,Phase>=ERecoveryPhase::LandingBurn?3.2:1.6);
    FVector C0=FVector::ZeroVector,C1=FVector::ZeroVector,C2=FVector::ZeroVector,AxialTorque=FVector::ZeroVector;
    for(auto& Engine:Engines)
    {
        Engine.ThrustN*=FuelScale;
        const FVector R=Engine.PositionFromBaseM-COM;
        AxialTorque+=FVector::CrossProduct(R,FVector(0,0,Engine.ThrustN));
        if(!Engine.bGimballed || Engine.ThrustN<1.)continue;
        for(const FVector A:{FVector(0,R.Z,-R.Y),FVector(-R.Z,0,R.X)})
        { C0+=A*A.X;C1+=A*A.Y;C2+=A*A.Z; }
    }
    const FVector Lambda=RecoveryActuators::SolveSymmetric(C0,C1,C2,DesiredTorque-AxialTorque);
    LastEngineForceBodyN=LastEngineMomentBodyNm=FVector::ZeroVector;
    for(auto& Engine:Engines)
    {
        const FVector R=Engine.PositionFromBaseM-COM;
        FVector Target=FVector::UpVector;
        if(Engine.bGimballed && Engine.ThrustN>1.)
        {
            FVector Side(FVector::DotProduct(FVector(0,R.Z,-R.Y),Lambda),FVector::DotProduct(FVector(-R.Z,0,R.X),Lambda),0);
            Side=Side.GetClampedToMaxSize(Engine.ThrustN*FMath::Tan(FMath::DegreesToRadians(RuntimeProfile->MaxGimbalDeg)));
            Target=FVector(Side.X,Side.Y,Engine.ThrustN).GetSafeNormal();
        }
        const double Angle=FMath::Acos(FMath::Clamp(FVector::DotProduct(Engine.DirectionBody,Target),-1.,1.));
        const double Fraction=Angle>1.e-8?FMath::Min(1-FMath::Exp(-Dt/.08),FMath::DegreesToRadians(RuntimeProfile->GimbalRateDegS)*Dt/Angle):1.;
        Engine.DirectionBody=FMath::Lerp(Engine.DirectionBody,Target,Fraction).GetSafeNormal();
        const FVector Force=Engine.DirectionBody*Engine.ThrustN;
        ApplyVehicleForce(ERecoveryForceKind::Engine,int32(&Engine-Engines.GetData()),Q.RotateVector(Force),(BasePositionM+Q.RotateVector(Engine.PositionFromBaseM))*100);
        LastEngineForceBodyN+=Force;LastEngineMomentBodyNm+=FVector::CrossProduct(R,Force);
        PeakEngineForceRatio=FMath::Max(PeakEngineForceRatio,Engine.ThrustN>1.?Force.Size()/Engine.ThrustN:0.);
        PeakAppliedGimbalDeg=FMath::Max(PeakAppliedGimbalDeg,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Engine.DirectionBody.Z,-1.,1.))));
    }
    AppliedGimbal=FVector(LastEngineForceBodyN.X,LastEngineForceBodyN.Y,0);
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

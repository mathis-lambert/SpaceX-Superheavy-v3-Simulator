#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::SelectAlternate(const FRecoveryDynamicsState& Dynamics)
{
    if(State.bAlternateRecovery)return;
    State.bAlternateRecovery=true;State.bCorrectiveBurn=false;State.TerminalPlan.bFeasible=false;
    // Simulator-authored Gulf zones, east of the launch site. Scores estimate
    // lateral impulse, not a certified six-degree-of-freedom reachable set.
    double Best=TNumericLimits<double>::Max();
    State.AlternateSitesM.Reset();State.AlternateDeltaVMps.Reset();State.AlternateReachable.Reset();
    const auto& N=State.Navigation;
    const double BrakeFuel=FMath::Max(State.LandingPrediction.FuelKg,Config.LandingReserveKg*.65);
    const double Spare=FMath::Max(0.,Dynamics.PropellantKg-BrakeFuel);
    State.AvailableDivertDeltaVMps=N.EngineIspS*RecoveryAtmosphere::G0*FMath::Loge(N.MassKg/FMath::Max(1.,N.MassKg-Spare));
    const double TurnDelay=3.+FMath::Acos(FMath::Clamp(Body.Rotation.GetUpVector().Z,-1.,1.))/RecoveryActuators::MaximumBodyRateRadS;
    const double Time=FMath::Max(0.,State.TimeToImpactS-TurnDelay);
    const double LateralAcceleration=FMath::Max(0.,13*Config.EngineThrustN/N.MassKg-N.Gravity)*.3;
    State.bSafeAlternateAvailable=false;
    for(double East:{20000.,30000.,40000.})for(double North:{-10000.,0.,10000.})
    {
        FVector Site=Config.LaunchWorldM+FVector(East,North,0);
        Site.Z=FMath::Sqrt(FMath::Max(0.,FMath::Square(FlightGeometry::EarthRadiusM)-Site.X*Site.X-Site.Y*Site.Y))-FlightGeometry::EarthRadiusM;
        const double Cost=FVector2D(Site-State.PredictedImpactM).Size()/FMath::Max(8.,State.TimeToImpactS);
        State.AlternateSitesM.Add(Site);State.AlternateDeltaVMps.Add(Cost);
        // Reserve vertical braking fuel and allow for attitude acquisition.
        // This is a conservative point-mass screen, not a certified 6-DOF plan.
        const bool Reachable=Cost<State.AvailableDivertDeltaVMps*.7 &&
            FVector2D(Site-State.PredictedImpactM).Size()<.25*LateralAcceleration*Time*Time;
        State.AlternateReachable.Add(Reachable);
        if(Reachable && Cost<Best){Best=Cost;State.AlternateTargetM=Site;State.bSafeAlternateAvailable=true;}
    }
    State.RequiredDivertDeltaVMps=State.bSafeAlternateAvailable?Best:0;
    if(!State.bSafeAlternateAvailable)State.AlternateTargetM=State.PredictedImpactM;
    Transition(ERecoveryPhase::Entry,State.bSafeAlternateAvailable?ERecoveryGuidanceReason::AlternateSelected:ERecoveryGuidanceReason::ImpactMitigation);
}

void FRecoveryGuidanceModel::GuideAlternate(const FRecoveryDynamicsState& Dynamics,double Dt,FVector& ForceAccel,FVector& TargetUp)
{
    const auto& N=State.Navigation;
    State.TargetPositionM=State.AlternateTargetM;
    State.Command.EngineCount=0;ForceAccel=FVector::ZeroVector;
    const FVector Relative=N.VelocityMps-WindAt(N.AltitudeM);
    TargetUp=Relative.Z<0?-Relative.GetSafeNormal():FVector::UpVector;
    // Sea-level endpoint only: a longitude threshold is not a water/terrain map.
    if(N.AltitudeM<=.5){Fail(ERecoveryGuidanceReason::EmergencyContact);return;}
    const double Rated=Config.EngineThrustN*N.EngineIspS/Config.SpecificImpulseSeaLevelS;
    int32 Count=13;if(Experiment.FailedEngine>=0)--Count;
    const double MaxAccel=Count*Rated/FMath::Max(1.,N.MassKg);
    const double Decel=FMath::Max(.1,FMath::Min(Config.LandingDecelerationMps2,MaxAccel*.65-N.Gravity));
    const double DownSpeed=FMath::Max(0.,-N.VerticalSpeedMps);
    const double TurnLead=1.+FMath::DegreesToRadians(N.TiltDeg)/RecoveryActuators::MaximumBodyRateRadS;
    const double Stopping=FMath::Square(FMath::Min(0.,N.VerticalSpeedMps))/(2*Decel)+
        DownSpeed*(Config.Engines.OpeningTimeConstantS*2+TurnLead);
    // Commit to braking early enough to turn and open the actual valves. Do not
    // alternate between tail-first coast and a horizontal divert axis at ignition.
    State.bAlternateBraking|=N.VerticalSpeedMps<0 && N.AltitudeM<Stopping+150;
    if(State.bAlternateBraking && State.bSafeAlternateAvailable)
    {
        const double Time=FMath::Max(1.,2*N.AltitudeM/FMath::Max(2.,DownSpeed));
        if(FVector2D(State.AlternateTargetM-State.PredictedImpactM).Size()>500+1.5*Time*Time)
        {
            // Revoke the early lateral estimate when terminal authority cannot
            // close the remaining miss. Continue impact-energy reduction.
            State.bSafeAlternateAvailable=false;
            State.AlternateReachable.Init(false,State.AlternateSitesM.Num());
            State.AlternateTargetM=State.PredictedImpactM;
            Transition(ERecoveryPhase::Entry,ERecoveryGuidanceReason::ImpactMitigation);
        }
    }
    FVector Lateral=State.bSafeAlternateAvailable?
        (State.AlternateTargetM-State.PredictedImpactM)*(2/FMath::Square(FMath::Max(10.,State.TimeToImpactS))):
        -N.VelocityMps*.3;
    Lateral.Z=0;
    if(!State.bAlternateBraking && N.DynamicPressurePa>200)
    {
        const double Authority=N.DynamicPressurePa*Config.BodySideAreaM2*Config.BodyNormalCoefficient/FMath::Max(1.,N.MassKg);
        TargetUp=(TargetUp+(-Lateral/FMath::Max(.1,Authority)).GetClampedToMaxSize(.2)).GetSafeNormal();
    }
    if(Dynamics.PropellantKg<=0)return; // Aerodynamic control remains available.
    const bool Brake=State.bAlternateBraking;
    const bool Divert=N.AltitudeM>15000 && Lateral.Size()>1 && Dynamics.PropellantKg>Config.LandingReserveKg*.5;
    if(!Brake && !Divert)return;
    const double DesiredVz=-FMath::Max(2.,FMath::Sqrt(2*Decel*FMath::Max(0.,N.AltitudeM-30)));
    // Once terminal braking starts, reducing impact energy takes priority over
    // chasing an old footprint. Bound tilt and use the core bank near touchdown.
    if(Brake){Lateral=-FVector(N.VelocityMps.X,N.VelocityMps.Y,0)*.25;}
    ForceAccel=Lateral.GetClampedToMaxSize(Brake?3.:MaxAccel*.35);
    ForceAccel.Z=Brake?FMath::Clamp(N.Gravity+Decel+(DesiredVz-N.VerticalSpeedMps)*1.2, N.Gravity*.7,MaxAccel*.9):N.Gravity*.25;
    if(Brake && N.AltitudeM<35)ForceAccel.Z=FMath::Clamp(N.Gravity+(-2.-N.VerticalSpeedMps)*1.5,0.,MaxAccel*.9);
    ForceAccel=ForceAccel.GetClampedToMaxSize(MaxAccel);TargetUp=ForceAccel.GetSafeNormal();
    if(FVector::DotProduct(TargetUp,Body.Rotation.GetUpVector())>.8)
        State.Command.EngineCount=Brake && ForceAccel.Size()*N.MassKg<3*Rated*.85?3:13;
    else ForceAccel=FVector::ZeroVector;
    if(Brake && Dynamics.ThrustN>1)State.LandingBurnSeconds+=Dt;
}

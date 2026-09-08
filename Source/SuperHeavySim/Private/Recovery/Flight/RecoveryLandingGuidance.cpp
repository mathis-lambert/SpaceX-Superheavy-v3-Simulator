#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::GuideLanding(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,
    double Dt,FVector& ForceAccel,FVector& TargetUp)
{
    const auto& N=State.Navigation;
    State.LandingBurnSeconds+=Dt;
    if(State.Phase==ERecoveryPhase::LandingBurn && N.AltitudeM<500 && N.HorizontalErrorM<40 &&
        FVector2D(N.VelocityMps).Size()<4 && N.TiltDeg<5 && N.HeadingErrorDeg<3)
        Transition(ERecoveryPhase::Capture,ERecoveryGuidanceReason::Capture);
    const double Centre=Dynamics.Mass.CentreFromBaseM;
    const FVector Position=N.BasePositionM+Body.Rotation.GetUpVector()*Centre;
    const FVector LugMid=(Config.CatchLugPlusM+Config.CatchLugMinusM)*.5;
    const FQuat CaptureQ=Config.TowerRotation*FQuat(FVector::UpVector,FMath::DegreesToRadians(Config.CaptureHeadingDeg));
    const FVector FittingTarget=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid)-Body.Rotation.RotateVector(LugMid-FVector(0,0,Centre));
    const double Height=FMath::Max(0.,N.AltitudeM-Config.CaptureWorldM.Z);
    const double PoseBlend=1-FMath::SmoothStep(30.,130.,Height);
    const FVector Goal=FMath::Lerp(Config.CaptureWorldM+FVector(0,0,Centre),FittingTarget,PoseBlend)-FVector(0,0,.20);
    const FVector Error=Goal-Position;
    const double AvailablePerEngine=Config.EngineThrustN*N.EngineIspS/Config.SpecificImpulseSeaLevelS;
    double CoreThrust=0,LandingThrust=0;
    for(int32 I=0;I<Dynamics.Engines.Num();++I)
    {
        if(RecoveryActuators::EngineRequested(Dynamics.Engines[I],I,3,External.Experiment.FailedEngine))CoreThrust+=AvailablePerEngine;
        if(RecoveryActuators::EngineRequested(Dynamics.Engines[I],I,13,External.Experiment.FailedEngine))LandingThrust+=AvailablePerEngine;
    }
    TerminalClock-=Dt;State.TerminalPlan.ElapsedS+=Dt;
    if(Height<1800 && N.VelocityMps.Size()<180 && TerminalClock<=0)
    {
        TerminalClock=.2;++State.TerminalReplans;
        FRecoveryTerminalInput Input;
        Input.PositionM=Position;Input.VelocityMps=N.VelocityMps;Input.TargetM=Goal;
        Input.AccelerationMps2=(Dynamics.AeroForceN+Body.Rotation.RotateVector(Dynamics.EngineForceBodyN))/N.MassKg-FVector(0,0,N.Gravity);
        Input.UpWorld=Body.Rotation.GetUpVector();Input.WindMps=WindAt(N.AltitudeM);
        Input.HeadingWorld=CaptureQ.GetForwardVector();
        Input.TowerWorldM=Config.TowerWorldM;Input.TowerRotation=Config.TowerRotation;Input.TowerHeightM=Config.TowerHeightM;
        Input.CaptureBaseHeightM=Config.CaptureWorldM.Z;Input.CentreFromBaseM=Centre;
        Input.FittingMidFromBaseM=LugMid;Input.TargetFittingWorldM=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid);
        Input.MassKg=N.MassKg;Input.FuelKg=Dynamics.PropellantKg;Input.CoreThrustN=CoreThrust;
        Input.LandingThrustN=LandingThrust;Input.IspS=N.EngineIspS;Input.MaxTiltDeg=Config.MaxTiltDeg;
        State.TerminalPlan=RecoveryTerminalGuidance::Plan(Input,Config);
        if(!State.TerminalPlan.bFeasible)++State.TerminalRejectedPlans;
    }
    if(State.TerminalPlan.bFeasible)
    {
        State.TerminalPlannedSeconds+=Dt;
        const auto& Plan=State.TerminalPlan;
        const double T=FMath::Min(Plan.ElapsedS,Plan.HorizonS);
        const FVector Net=Plan.AccelerationAt(T)+(Plan.PositionAt(T)-Position)*.5+(Plan.VelocityAt(T)-N.VelocityMps)*1.5;
        ForceAccel=RecoveryTerminalGuidance::RequiredThrustAcceleration(Net,N.VelocityMps,Position,N.MassKg,WindAt(N.AltitudeM),Config);
    }
    else
    {
        // Energy braking remains active before a feasible terminal transfer is
        // available. It targets the catch altitude, with no fixed hover shelf.
        State.TerminalBrakingSeconds+=Dt;
        const double Decel=FMath::Min(Config.LandingDecelerationMps2,.7*(CoreThrust/N.MassKg-N.Gravity));
        const double DesiredVz=Error.Z>0?FMath::Min(3.,Error.Z*.5):-FMath::Min(FMath::Sqrt(2*FMath::Max(1.,Decel)*Height),FMath::Max(.35,Height*.7));
        const FVector DesiredVelocity=FVector(Error.X,Error.Y,0).GetClampedToMaxSize(200)*.25;
        FVector Net=(DesiredVelocity-N.VelocityMps)*.3;
        Net.Z=FMath::Clamp((DesiredVz-N.VelocityMps.Z)*.8,-4.,LandingThrust/N.MassKg-N.Gravity);
        ForceAccel=Net-Dynamics.AeroForceN/N.MassKg+FVector(0,0,N.Gravity);
        ForceAccel.Z=FMath::Max(N.Gravity*.5,ForceAccel.Z);
        const FVector AirVelocity=N.VelocityMps-WindAt(N.AltitudeM);
        const double Speed=FMath::Max(1.,AirVelocity.Size());
        const double NormalAuthority=N.DynamicPressurePa*Config.BodySideAreaM2*Config.BodyNormalCoefficient/N.MassKg;
        const double CdA=Config.DragAreaM2*Config.TailFirstDragCoefficient+3*Config.GridFinAreaM2*Config.GridFinDragCoefficient;
        const FVector Axial=-N.DynamicPressurePa*CdA*AirVelocity/(Speed*N.MassKg);
        const double Coupled=ForceAccel.Z-NormalAuthority;
        FVector Tilt;
        if(FMath::Abs(Coupled)>ForceAccel.Z*.2)
            Tilt=FVector(Net.X-Axial.X+NormalAuthority*AirVelocity.X/Speed,Net.Y-Axial.Y+NormalAuthority*AirVelocity.Y/Speed,0)/Coupled;
        else Tilt=-FVector(AirVelocity.X,AirVelocity.Y,0)/FMath::Max(20.,FMath::Abs(AirVelocity.Z));
        if(Speed<50)Tilt=FVector(ForceAccel.X,ForceAccel.Y,0)/ForceAccel.Z;
        ForceAccel=FVector(Tilt.X,Tilt.Y,1)*ForceAccel.Z;
    }
    // These bound the command only. Physical valves, gimbals and rigid-body
    // motion remain independent and may be unable to achieve the request.
    ForceAccel.Z=FMath::Max(.1,ForceAccel.Z);
    const FVector Side=FVector(ForceAccel.X,ForceAccel.Y,0).GetClampedToMaxSize(ForceAccel.Z*FMath::Tan(FMath::DegreesToRadians(Config.MaxTiltDeg)));
    ForceAccel=FVector(Side.X,Side.Y,ForceAccel.Z);
    TargetUp=ForceAccel.GetSafeNormal();
    const double ThreeEngineDemand=CoreThrust>0?ForceAccel.Size()*N.MassKg/CoreThrust:1.e10;
    LandingEngineGroup=LandingEngineGroup>=13?(ThreeEngineDemand<.80?3:13):(ThreeEngineDemand>.96?13:3);
    State.Command.EngineCount=LandingEngineGroup;
        if(State.Phase==ERecoveryPhase::Capture)
        {
            const FVector Up=Body.Rotation.GetUpVector();
            const FVector RailTarget=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid);
            const FVector AtRail=N.BasePositionM+Up*((RailTarget.Z-N.BasePositionM.Z)/FMath::Max(.1,Up.Z));
            const double Across=FMath::Abs(Config.TowerRotation.UnrotateVector(AtRail-RailTarget).Y);
            const double SafeGap=4.5/FMath::Max(.1,Up.Z)+.55+Across+.05;
            State.ArmClosure=FMath::Min(FMath::Clamp((60.-Height)/25.,0.,1.),FMath::Clamp((10.-SafeGap)/4.85,0.,1.));
            // Transfer weight on the first verified fitting contact. Continuing
            // hover thrust would hold the other fitting a few centimetres above
            // its rail in crosswind. State.Navigation.Gravity settles both supports physically.
            if(External.SupportContactCount>0 && State.Navigation.CatchLugErrorM<0.5 && State.Navigation.VelocityMps.Size()<1.5 && State.Navigation.TiltDeg<3 && !State.bContactShutdown)
            {
                State.bContactShutdown=true;
            }
            if(State.bContactShutdown) { State.Command.EngineCount=0;ForceAccel=FVector::ZeroVector; }
            const bool Supported=State.bContactShutdown && External.SupportContactCount==2 && State.Navigation.VelocityMps.Size()<0.5 && State.Navigation.TiltDeg<4.;
            State.SettledContactSeconds=Supported?State.SettledContactSeconds+Dt:0;
            if(State.SettledContactSeconds>=0.6)
            {
                State.CaptureErrorAtLatch=State.Navigation.HorizontalErrorM; State.CaptureSpeedAtLatch=State.Navigation.VelocityMps.Size(); State.CaptureTiltAtLatch=State.Navigation.TiltDeg;
                State.CaptureHeadingAtLatch=State.Navigation.HeadingErrorDeg; State.CaptureLugAtLatch=State.Navigation.CatchLugErrorM; State.LatchPositionM=State.Navigation.BasePositionM;
                State.ArmClosure=1;
                Transition(ERecoveryPhase::Captured,ERecoveryGuidanceReason::Captured);
                State.Command.EngineCount=0;
            }
        }
}

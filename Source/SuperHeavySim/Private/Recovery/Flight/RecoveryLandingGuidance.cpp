#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::GuideLanding(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,
    double Dt,FVector& ForceAccel,FVector& TargetUp)
{
    const auto& N=State.Navigation;
    if(Dynamics.ThrustN>1)State.LandingBurnSeconds+=Dt;
    if(State.Phase==ERecoveryPhase::LandingBurn && State.TerminalPlan.bFeasible && N.AltitudeM<350 &&
        N.HorizontalErrorM<150 && N.HeadingErrorDeg<10)
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
    TerminalClock=FMath::Max(-Dt,TerminalClock-Dt);State.TerminalPlan.ElapsedS+=Dt;
    const bool CapabilityChanged=State.TerminalPlan.bFeasible && CoreThrust<State.TerminalInput.CoreThrustN*.85;
    const bool TrackingLost=State.TerminalPlan.bFeasible &&
        ((State.TerminalReferenceM-Position).Size()>5 || (State.TerminalReferenceVelocityMps-N.VelocityMps).Size()>2);
    const bool NeedPlan=!State.TerminalPlan.bFeasible || State.TerminalPlan.ElapsedS>=State.TerminalPlan.HorizonS || CapabilityChanged || TrackingLost;
    if(!State.bContactShutdown && NeedPlan && Height<1800 && N.VelocityMps.Size()<180 && TerminalClock<=0)
    {
        TerminalClock+=.2;++State.TerminalReplans;
        FRecoveryTerminalInput Input;
        Input.PositionM=Position;Input.VelocityMps=N.VelocityMps;Input.TargetM=Goal;
        // Include fins, vents and radial gravity in the incoming net force.
        for(const auto& Force:Dynamics.Forces)Input.AccelerationMps2+=Force.ForceN/N.MassKg;
        Input.UpWorld=Body.Rotation.GetUpVector();Input.WindMps=WindAt(N.AltitudeM);
        Input.HeadingWorld=CaptureQ.GetForwardVector();
        Input.TowerWorldM=Config.TowerWorldM;Input.TowerRotation=Config.TowerRotation;Input.TowerHeightM=Config.TowerHeightM;
        Input.CaptureBaseHeightM=Config.CaptureWorldM.Z;Input.CentreFromBaseM=Centre;
        Input.FittingMidFromBaseM=LugMid;Input.TargetFittingWorldM=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid);
        Input.MassKg=N.MassKg;Input.FuelKg=Dynamics.PropellantKg;Input.CoreThrustN=CoreThrust;
        Input.LandingThrustN=LandingThrust;Input.IspS=N.EngineIspS;Input.MaxTiltDeg=Config.MaxTiltDeg;
        FVector ArrivalThrust;
        if(RecoveryTerminalGuidance::TryRequiredThrustAcceleration(FVector::ZeroVector,Input.TargetVelocityMps,
            Config.CaptureWorldM,N.MassKg,Input.WindMps,Config,FVector::UpVector,ArrivalThrust))
        {
            const FQuat ArrivalQ=FRotationMatrix::MakeFromZX(ArrivalThrust.GetSafeNormal(),Input.HeadingWorld).ToQuat();
            Input.TargetM=Input.TargetFittingWorldM-ArrivalQ.RotateVector(LugMid-FVector(0,0,Centre))-FVector(0,0,.2);
        }
        const auto Candidate=RecoveryTerminalGuidance::Plan(Input,Config);
        State.TerminalCandidate=Candidate;
        if(!Candidate.bFeasible)++State.TerminalRejectedPlans;
        else
        {
            State.TerminalPlan=Candidate;
            State.TerminalInput=Input;
        }
        if(CapabilityChanged && !Candidate.bFeasible)State.TerminalPlan.bFeasible=false;
    }
    bool TrackPlan=State.TerminalPlan.bFeasible && !State.bContactShutdown;
    if(TrackPlan)
    {
        const auto& Plan=State.TerminalPlan;
        const double T=FMath::Min(Plan.ElapsedS,Plan.HorizonS);
        // Retain the accepted arrival deadline and reference history. Near the
        // rails regulate the physical fittings as the mass centre moves.
        const double FittingBlend=1-FMath::SmoothStep(5.,25.,Height);
        const FVector Reference=Plan.PositionAt(T)+(Goal-Plan.PositionAt(Plan.HorizonS))*FittingBlend;
        State.TerminalReferenceM=Reference;State.TerminalReferenceVelocityMps=Plan.VelocityAt(T);
        State.TerminalPeakTrackingErrorM=FMath::Max(State.TerminalPeakTrackingErrorM,(Reference-Position).Size());
        const FVector PositionError=Reference-Position,VelocityError=Plan.VelocityAt(T)-N.VelocityMps;
        const FVector Feedback=PositionError*FVector(.06,.06,.4)+VelocityError*FVector(.45,.45,1.2);
        FVector Net=Plan.AccelerationAt(T)+Feedback;
        const double ValveLead=FMath::Min(Config.Engines.OpeningTimeConstantS,Plan.HorizonS*.25);
        Net.Z=Plan.AccelerationAt(FMath::Min(T+ValveLead,Plan.HorizonS)).Z+Feedback.Z;
        TrackPlan=RecoveryTerminalGuidance::TryRequiredThrustAcceleration(Net,N.VelocityMps,Position,N.MassKg,
            WindAt(N.AltitudeM),Config,Body.Rotation.GetUpVector(),ForceAccel);
        if(TrackPlan)
        {
            State.TerminalPlannedSeconds+=Dt;
            // Feed the reference turn rate to the bounded moment controller.
            // This requests gimbal/fin torque; it never sets body angular speed.
            constexpr double RateInterval=.05;
            FVector FutureForce;
            FVector FutureNet=Plan.AccelerationAt(FMath::Min(T+RateInterval,Plan.HorizonS))+Feedback;
            FutureNet.Z=Plan.AccelerationAt(FMath::Min(T+RateInterval+ValveLead,Plan.HorizonS)).Z+Feedback.Z;
            if(RecoveryTerminalGuidance::TryRequiredThrustAcceleration(FutureNet,N.VelocityMps+Net*RateInterval,
                Position+N.VelocityMps*RateInterval,N.MassKg,WindAt(N.AltitudeM),Config,ForceAccel.GetSafeNormal(),FutureForce))
                State.Command.TargetAngularVelocityWorldRadS=FVector::CrossProduct(ForceAccel.GetSafeNormal(),FutureForce.GetSafeNormal())/RateInterval;
        }
    }
    if(!TrackPlan)
    {
        // Energy braking remains active before a feasible terminal transfer is
        // available. It targets the catch altitude, with no fixed hover shelf.
        if(!State.bContactShutdown)State.TerminalBrakingSeconds+=Dt;
        const double Decel=FMath::Min(Config.LandingDecelerationMps2,.7*(CoreThrust/N.MassKg-N.Gravity));
        // Vertical braking must leave time to remove lateral displacement and
        // velocity. A vertical-only stopping curve can reach the rail altitude
        // hundreds of metres away from the opening.
        const double LateralAcceleration=FMath::Max(.25,N.Gravity*FMath::Tan(FMath::DegreesToRadians(Config.MaxTiltDeg))*.7);
        const double LateralTime=2*FMath::Sqrt(FVector2D(Error).Size()/LateralAcceleration)+FVector2D(N.VelocityMps).Size()/LateralAcceleration;
        const double DescentLimit=2*Height/FMath::Max(1.,LateralTime);
        const double DesiredVz=Error.Z>0?FMath::Min(3.,Error.Z*.5):-FMath::Min3(FMath::Sqrt(2*FMath::Max(1.,Decel)*Height),FMath::Max(.35,Height*.7),FMath::Max(.35,DescentLimit));
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
        // Loaded rails must not withdraw in response to the initial settling
        // motion. Hold their command during weight transfer and support.
        if(!State.bContactShutdown && External.SupportContactCount==0)
            State.ArmClosure=FMath::Min(FMath::Clamp((60.-Height)/25.,0.,1.),FMath::Clamp((10.-SafeGap)/4.85,0.,1.));
        // Transfer weight on the first verified fitting contact. Continuing
        // hover thrust would hold the other fitting a few centimetres above
        // its rail in crosswind. State.Navigation.Gravity settles both supports physically.
        if(External.SupportContactCount>0 && State.Navigation.CatchLugErrorM<0.5 && State.Navigation.VelocityMps.Size()<1.5 && State.Navigation.TiltDeg<3 && !State.bContactShutdown)
        {
            State.bContactShutdown=true;
        }
        if(State.bContactShutdown) { State.Command.EngineCount=0;ForceAccel=FVector::ZeroVector; }
        const bool Supported=State.bContactShutdown && External.SupportContactCount==2 &&
            State.Navigation.VelocityMps.Size()<.05 && Body.AngularVelocityWorldRadS.Size()<.001 && State.Navigation.TiltDeg<4.;
        State.SettledContactSeconds=Supported?State.SettledContactSeconds+Dt:0;
        if(State.SettledContactSeconds>=1.)
        {
            State.CaptureErrorAtLatch=State.Navigation.HorizontalErrorM; State.CaptureSpeedAtLatch=State.Navigation.VelocityMps.Size(); State.CaptureTiltAtLatch=State.Navigation.TiltDeg;
            State.CaptureHeadingAtLatch=State.Navigation.HeadingErrorDeg; State.CaptureLugAtLatch=State.Navigation.CatchLugErrorM; State.LatchPositionM=State.Navigation.BasePositionM;
            Transition(ERecoveryPhase::Captured,ERecoveryGuidanceReason::Captured);
            State.Command.EngineCount=0;
        }
    }
}

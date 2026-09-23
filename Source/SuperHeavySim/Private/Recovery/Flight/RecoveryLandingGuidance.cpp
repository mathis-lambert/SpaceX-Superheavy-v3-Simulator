#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryRailSupport.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::GuideLanding(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,
    double Dt,FVector& ForceAccel,FVector& TargetUp)
{
    const auto& N=State.Navigation;
    if(State.FirstContactTimeS<0 && External.SupportContactCount>0)
    {
        State.FirstContactTimeS=State.SampleTimeS;
        // The previous force sample precedes the solver's first support impulse.
        // Report that incoming velocity, not the already arrested contact state.
        State.FirstContactSpeedMps=Dynamics.Body.VelocityMps.Size();
        State.FirstContactVerticalSpeedMps=Dynamics.Body.VelocityMps.Z;
        State.FirstContactAngularSpeedDegS=FMath::RadiansToDegrees(Dynamics.Body.AngularVelocityWorldRadS.Size());
        State.FirstContactTiltDeg=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dynamics.Body.Rotation.GetUpVector().Z,-1.,1.)));
    }
    if(State.FirstContactTimeS<0 && N.AltitudeM-Config.CaptureWorldM.Z<100 && N.VelocityMps.Size()<5)
        State.LowSlowApproachSeconds+=Dt;
    if(Dynamics.ThrustN>1)State.LandingBurnSeconds+=Dt;
    if(State.Phase==ERecoveryPhase::LandingBurn && State.TerminalPlan.bFeasible && N.AltitudeM<350 &&
        N.HorizontalErrorM<150 && N.HeadingErrorDeg<10)
        Transition(ERecoveryPhase::Capture,ERecoveryGuidanceReason::Capture);
    const double Centre=Dynamics.Mass.CentreFromBaseM;
    const FVector Position=N.BasePositionM+Body.Rotation.GetUpVector()*Centre;
    const FVector LugMid=(Config.CatchLugPlusM+Config.CatchLugMinusM)*.5;
    const FQuat CaptureQ=Config.TowerRotation*FQuat(FVector::UpVector,FMath::DegreesToRadians(Config.CaptureHeadingDeg));
    const double Height=FMath::Max(0.,N.AltitudeM-Config.CaptureWorldM.Z);
    FVector ArrivalThrust;
    RecoveryTerminalGuidance::TryRequiredThrustAcceleration(FVector::ZeroVector,FVector(0,0,-Config.ContactDescentSpeedMps),
        Config.CaptureWorldM,N.MassKg,WindAt(Config.CaptureWorldM.Z),Config,FVector::UpVector,ArrivalThrust);
    const FQuat ArrivalQ=FRotationMatrix::MakeFromZX(ArrivalThrust.IsNearlyZero()?FVector::UpVector:ArrivalThrust.GetSafeNormal(),CaptureQ.GetForwardVector()).ToQuat();
    // Target the equilibrium arrival attitude. Chasing the current tilted
    // fitting with the COM controller couples rotation back into translation.
    const FVector Goal=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid)-ArrivalQ.RotateVector(LugMid-FVector(0,0,Centre))-FVector(0,0,.025);
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
        Input.TargetVelocityMps=FVector(0,0,-Config.ContactDescentSpeedMps);
        // Include fins, vents and radial gravity in the incoming net force.
        for(const auto& Force:Dynamics.Forces)Input.AccelerationMps2+=Force.ForceN/N.MassKg;
        Input.UpWorld=Body.Rotation.GetUpVector();Input.WindMps=WindAt(N.AltitudeM);
        Input.HeadingWorld=CaptureQ.GetForwardVector();
        Input.TowerWorldM=Config.TowerWorldM;Input.TowerRotation=Config.TowerRotation;Input.TowerHeightM=Config.TowerHeightM;
        Input.CaptureBaseHeightM=Config.CaptureWorldM.Z;Input.CentreFromBaseM=Centre;
        Input.FittingMidFromBaseM=LugMid;Input.TargetFittingWorldM=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid);
        Input.MassKg=N.MassKg;Input.FuelKg=Dynamics.PropellantKg;Input.CoreThrustN=CoreThrust;
        Input.LandingThrustN=LandingThrust;Input.IspS=N.EngineIspS;Input.MaxTiltDeg=Config.MaxTiltDeg;
        const auto Candidate=RecoveryTerminalGuidance::Plan(Input,Config);
        if(!State.TerminalPlan.bFeasible)State.TerminalInput=Input;
        State.TerminalCandidate=Candidate;
        if(!Candidate.bFeasible)++State.TerminalRejectedPlans;
        else
        {
            State.TerminalPlan=Candidate;
            State.TerminalInput=Input;
        }
        if(!Candidate.bFeasible && (CapabilityChanged || TrackingLost || State.TerminalPlan.ElapsedS>=State.TerminalPlan.HorizonS))State.TerminalPlan.bFeasible=false;
    }
    bool TrackPlan=State.TerminalPlan.bFeasible && !State.bContactShutdown;
    if(TrackPlan)
    {
        State.RejectedPlanSeconds=0;
        const auto& Plan=State.TerminalPlan;
        const double T=FMath::Min(Plan.ElapsedS,Plan.HorizonS);
        // Retain the accepted arrival deadline and reference history. Near the
        // rails regulate the physical fittings as the mass centre moves.
        const double FittingBlend=1-FMath::SmoothStep(30.,130.,Height);
        const FVector Reference=Plan.PositionAt(T)+(Goal-Plan.PositionAt(Plan.HorizonS))*FittingBlend;
        State.TerminalReferenceM=Reference;State.TerminalReferenceVelocityMps=Plan.VelocityAt(T);
        State.TerminalPeakTrackingErrorM=FMath::Max(State.TerminalPeakTrackingErrorM,(Reference-Position).Size());
        const FVector PositionError=Reference-Position,VelocityError=Plan.VelocityAt(T)-N.VelocityMps;
        // Keep lateral feedback slower than the physical attitude response.
        // Excessive crossrange gain drives a late oscillation at weight transfer.
        // All gains act in tower coordinates, independently of compass heading.
        const FVector Feedback=Config.TowerRotation.RotateVector(
            Config.TowerRotation.UnrotateVector(PositionError)*FVector(.06,.06,.4)+
            Config.TowerRotation.UnrotateVector(VelocityError)*FVector(.35,.35,1.2));
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
        State.RejectedPlanSeconds+=Dt;
        if(State.RejectedPlanSeconds>12 && Height<1800 && State.TerminalRejectedPlans>20 && N.HorizontalErrorM>300)
        {SelectAlternate(Dynamics);GuideAlternate(Dynamics,Dt,ForceAccel,TargetUp);return;}
        // Energy braking remains active before a feasible terminal transfer is
        // available. It targets the catch altitude, with no fixed hover shelf.
        if(!State.bContactShutdown)State.TerminalBrakingSeconds+=Dt;
        const double Decel=FMath::Min(Config.LandingDecelerationMps2,.7*(CoreThrust/N.MassKg-N.Gravity));
        // Vertical braking must leave time to remove lateral displacement and
        // velocity. A vertical-only stopping curve can reach the rail altitude
        // hundreds of metres away from the opening.
        const double LateralAcceleration=FMath::Max(.25,N.Gravity*FMath::Tan(FMath::DegreesToRadians(Config.MaxTiltDeg))*.7);
        const double LateralTime=1.15*FMath::Max(
            RecoveryLanding::MinimumTransferTime(Error.X,N.VelocityMps.X,LateralAcceleration),
            RecoveryLanding::MinimumTransferTime(Error.Y,N.VelocityMps.Y,LateralAcceleration))+2*Config.Engines.OpeningTimeConstantS;
        const double DescentLimit=2*Height/FMath::Max(1.,LateralTime);
        const double DesiredVz=Error.Z>0?FMath::Min(3.,Error.Z*.5):-FMath::Min3(FMath::Sqrt(2*FMath::Max(1.,Decel)*Height),FMath::Max(.35,Height*.7),FMath::Max(.35,DescentLimit));
        // Lateral approach speed must fit inside the remaining stopping
        // distance on each axis. A proportional 50 m/s target kept accelerating
        // toward the mast after the main vertical braking pulse had ended.
        const auto ClosingAcceleration=[&](double Distance,double Velocity)
        {
            const double EnergySpeed=FMath::Sqrt(2*LateralAcceleration*FMath::Abs(Distance));
            const bool EnergyLimited=EnergySpeed<.4*FMath::Abs(Distance);
            const double Desired=FMath::Sign(Distance)*(EnergyLimited?EnergySpeed:.4*FMath::Abs(Distance));
            const double Slope=EnergyLimited?LateralAcceleration/FMath::Max(.01,EnergySpeed):.4;
            // Feed forward the deceleration of the closing-speed envelope.
            // Velocity feedback alone follows it late and overshoots the rails.
            return -Slope*Velocity+.65*(Desired-Velocity);
        };
        FVector Net(ClosingAcceleration(Error.X,N.VelocityMps.X),ClosingAcceleration(Error.Y,N.VelocityMps.Y),0);
        Net.Z=FMath::Clamp((DesiredVz-N.VelocityMps.Z)*.8,-4.,FMath::Min(Config.LandingDecelerationMps2,LandingThrust/N.MassKg-N.Gravity+Dynamics.AeroForceN.Z/N.MassKg));
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
    // Near the rails, vector engine thrust for translation and assign residual
    // pitch/roll moment to the gas jets. This allows an upright fitting contact
    // without demanding that the entire vehicle lean for every lateral correction.
    const FVector LocalDemand=Body.Rotation.UnrotateVector(ForceAccel*N.MassKg);
    const double TranslationMoment=FVector2D(LocalDemand).Size()*Dynamics.Mass.CentreFromBaseM;
    const double JetMargin=Config.ReactionControlTorqueNm*Dynamics.ReactionPressureEfficiency*.25;
    const double Translation=(!External.Experiment.bReactionJetsDisabled && Dynamics.RcsPropellantKg>300)?
        (1-FMath::SmoothStep(1.,8.,Height))*(1-FMath::SmoothStep(2.,5.,N.VelocityMps.Size()))*
        FMath::Min(1.,JetMargin/FMath::Max(1.,TranslationMoment)):0.;
    State.Command.GimbalTranslationWeight=Translation;
    TargetUp=FMath::Lerp(TargetUp,FVector::UpVector,Translation).GetSafeNormal();
    State.Command.TargetAngularVelocityWorldRadS*=1-Translation;
    const double ThreeEngineDemand=CoreThrust>0?ForceAccel.Size()*N.MassKg/CoreThrust:1.e10;
    LandingEngineGroup=LandingEngineGroup>=13?(ThreeEngineDemand<.80?3:13):(ThreeEngineDemand>.96?13:3);
    State.Command.EngineCount=LandingEngineGroup;
    if(State.Phase==ERecoveryPhase::Capture)
    {
        const FVector Up=Body.Rotation.GetUpVector();
        const FVector RailTarget=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid);
        const FVector AtRail=N.BasePositionM+Up*((RailTarget.Z-N.BasePositionM.Z)/FMath::Max(.1,Up.Z));
        const FVector RailOffset=Config.TowerRotation.UnrotateVector(AtRail-RailTarget);
        const double Across=FMath::Abs(RailOffset.Y);
        // Reserve clearance for the frame below the rail and the small roll
        // while the second fitting takes weight after the first contact.
        const FVector LocalUp=Config.TowerRotation.UnrotateVector(Up);
        const double Slope=LocalUp.Y/FMath::Max(.1,Up.Z);
        // Clear the entire truss, including its lower chord under the moving
        // rail. A rail-height-only test let a rolled hull brush that chord.
        const double RailToFrame=RecoveryTowerGeometry::RailCentreHeightM+.09+.18;
        const double LowerAcross=FMath::Abs(RailOffset.Y-Slope*(RailToFrame+.85));
        const double UpperAcross=FMath::Abs(RailOffset.Y-Slope*(RailToFrame-.85));
        // Retain hull clearance while leaving both fittings over the rails.
        // The slower transverse arrival needs 12 cm of settling reserve.
        const double SafeGap=4.5/FMath::Max(.1,Up.Z)+.55+FMath::Max3(Across,LowerAcross,UpperAcross)+.12;
        // Loaded rails must not withdraw in response to the initial settling
        // motion. Hold their command during weight transfer and support.
        if(!State.bContactShutdown && External.SupportContactCount==0)
            State.ArmClosure=FMath::Min(FMath::Clamp((60.-Height)/25.,0.,1.),
                RecoveryTowerGeometry::ClosureForGap(SafeGap,RailOffset.X,RecoveryTowerGeometry::HalfArmM-RecoveryContactGeometry::RailCentreOffsetM));
        // Transfer weight on the first verified fitting contact. Continuing
        // hover thrust would hold the other fitting a few centimetres above
        // its rail in crosswind. Longitudinal offset is acceptable within the
        // usable rail span: centring a supported vehicle with thrust can drive
        // it into the arm frame. Gravity settles both supports physically.
        if(External.SupportContactCount>0 && RecoveryContactGeometry::OnUsableRailSpan(RailOffset.X) && Across<.4 &&
            N.HeadingErrorDeg<2 && N.VelocityMps.Size()<1.5 && N.TiltDeg<3 && !State.bContactShutdown)
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

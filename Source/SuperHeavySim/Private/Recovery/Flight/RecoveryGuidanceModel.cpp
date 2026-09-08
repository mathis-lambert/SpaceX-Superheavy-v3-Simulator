#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::Guide(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,double Dt)
{
    PredictorClock+=Dt;
    if(PredictorClock>=0.25) { PredictorClock-=0.25; PredictBallistic(); }
    const FVector Downrange=Config.TowerRotation.GetForwardVector();
    const double AvailablePerEngine=Config.EngineThrustN*State.Navigation.EngineIspS/Config.SpecificImpulseSeaLevelS;
    FVector ForceAccel=FVector::ZeroVector,TargetUp=FVector::UpVector;
    State.Command.EngineCount=0;
    State.TargetPositionM=Config.CaptureWorldM;
    if(State.Phase==ERecoveryPhase::Ascent)
    {
        const double Pitch=FMath::DegreesToRadians(Config.AscentPitchDeg)*FMath::SmoothStep(10.,Config.AscentDurationS,State.PhaseTimeS);
        TargetUp=FVector::UpVector*FMath::Cos(Pitch)+Downrange*FMath::Sin(Pitch);
        double Load=FMath::Min(33*AvailablePerEngine/State.Navigation.MassKg,Config.AscentMaxAccelerationMps2);
        if(State.Navigation.DynamicPressurePa>35000) Load*=FMath::Clamp(35000./State.Navigation.DynamicPressurePa,0.65,1.);
        State.Command.EngineCount=33; ForceAccel=TargetUp*Load;
        if(State.PhaseTimeS>=Config.AscentDurationS || Dynamics.PropellantKg<Config.LandingReserveKg+Config.BoostbackReserveKg)
        {
            State.bSeparationRequested=true;
            Transition(ERecoveryPhase::Separation,TEXT("MECO / stage separation / return attitude"));
        }
    }
    if(State.Phase==ERecoveryPhase::Separation || State.Phase==ERecoveryPhase::Boostback)
    {
        const double Tgo=FMath::Max(40.,State.TimeToImpactS);
        FVector DV=(Config.CaptureWorldM-State.PredictedImpactM)*(1.18/Tgo); DV.Z=0;
        const double ApogeeError=Config.ApogeeM-State.Navigation.AltitudeM;
        const double DesiredVz=FMath::Sign(ApogeeError)*FMath::Sqrt(2*State.Navigation.Gravity*FMath::Abs(ApogeeError))*0.75;
        ForceAccel=DV/6.; ForceAccel.Z=(DesiredVz-State.Navigation.VerticalSpeedMps)/8.+State.Navigation.Gravity;
        TargetUp=ForceAccel.GetSafeNormal();
        State.Command.EngineCount=State.Phase==ERecoveryPhase::Boostback ? 13 : 0;
        if(State.Phase==ERecoveryPhase::Separation && External.bSeparated && State.PhaseTimeS>2 && FVector::DotProduct(TargetUp,Body.Rotation.GetUpVector())>0.97)
        {
            State.BoostbackIgnitionAltitudeM=State.Navigation.AltitudeM; State.BoostbackDownrangeM=State.Navigation.HorizontalErrorM;
            Transition(ERecoveryPhase::Boostback,TEXT("13-engine boostback / solving ballistic return")); State.Command.EngineCount=13;
        }
        if(State.Phase==ERecoveryPhase::Boostback)
        {
            State.BoostbackSeconds+=Dt;
            const double PredictedApogee=State.Navigation.AltitudeM+FMath::Square(FMath::Max(0.,State.Navigation.VerticalSpeedMps))/(2*State.Navigation.Gravity);
            if(State.PhaseTimeS>5 && State.PredictedMissM<500 && PredictedApogee<Config.ApogeeM+3000)
            { Transition(ERecoveryPhase::Coast,TEXT("Boostback cutoff / unpowered coast to apogee")); State.Command.EngineCount=0; }
            else if(Dynamics.PropellantKg<Config.LandingReserveKg)
            { Transition(ERecoveryPhase::Aborted,TEXT("Boostback depleted landing reserve")); Fail(State.Events.Last().Message); State.Command.EngineCount=0; }
        }
    }
    if(State.Phase==ERecoveryPhase::Coast && State.Navigation.VerticalSpeedMps<0 && State.Navigation.DynamicPressurePa>200)
        Transition(ERecoveryPhase::Entry,TEXT("Engines OFF / atmospheric descent / grid-fin guidance"));
    if(State.Phase==ERecoveryPhase::Coast || State.Phase==ERecoveryPhase::Entry)
    {
        State.Command.EngineCount=0; ForceAccel=FVector::ZeroVector;
        // Tail-first attitude and aerodynamic correction of the predicted entry point.
        const FVector Rel=State.Navigation.VelocityMps-WindAt(State.Navigation.AltitudeM);
        TargetUp=Rel.Z<-30 ? -Rel.GetSafeNormal() : FVector::UpVector;
        if(State.Phase==ERecoveryPhase::Entry)
        {
            const double LookAhead=FMath::Max(8.,State.TimeToImpactS);
            FVector DesiredA=(Config.CaptureWorldM-State.PredictedImpactM)*(2./(LookAhead*LookAhead)); DesiredA.Z=0;
            const double Authority=State.Navigation.DynamicPressurePa*Config.BodySideAreaM2*Config.BodyNormalCoefficient/FMath::Max(1.,State.Navigation.MassKg);
            FVector Correction=(-DesiredA/FMath::Max(0.1,Authority)).GetClampedToMaxSize(FMath::Tan(FMath::DegreesToRadians(Config.MaxEntryAngleDeg)));
            TargetUp=(TargetUp+Correction).GetSafeNormal();
        }
        // Ignition follows energy and available deceleration, not a fixed timer.
        if(State.Navigation.VerticalSpeedMps<-20 && State.Navigation.AltitudeM<Config.LandingIgnitionCeilingM && State.Navigation.AltitudeM-Config.CaptureWorldM.Z<=State.Navigation.BrakingDistanceM+Config.LandingBurnMarginM)
        {
            State.LandingIgnitionAltitudeM=State.Navigation.AltitudeM;
            Transition(ERecoveryPhase::LandingBurn,TEXT("Landing burn / 13 to 3 Raptor engines"));
        }
    }
    if(State.Phase==ERecoveryPhase::LandingBurn || State.Phase==ERecoveryPhase::Capture)
    {
        State.LandingBurnSeconds+=Dt;
        if(State.Phase==ERecoveryPhase::LandingBurn && State.Navigation.AltitudeM<500 && State.Navigation.HorizontalErrorM<40 &&
            FVector2D(State.Navigation.VelocityMps).Size()<4 && State.Navigation.TiltDeg<5 && State.Navigation.HeadingErrorDeg<3)
            Transition(ERecoveryPhase::Capture,TEXT("Final descent / catch-fittings and heading alignment"));
        // Keep the base above the tower until the terminal corridor is acquired.
        // This gate is geometric and re-evaluated from measured flight state.
        // A crosswind requires a small steady lean. Rejecting that necessary
        // attitude would leave the vehicle hovering until its reserve runs out.
        if(State.Phase==ERecoveryPhase::Capture && State.Navigation.HorizontalErrorM<2.0 && FVector2D(State.Navigation.VelocityMps).Size()<0.7 && State.Navigation.TiltDeg<4. && State.Navigation.HeadingErrorDeg<2.) State.bApproachAligned=true;
        const double Clearance=State.Phase==ERecoveryPhase::LandingBurn || !State.bApproachAligned ? Config.TowerHeightM+30 : 0;
        // Translate the centre-of-mass target so the actual pair of fittings,
        // rather than the base axis, reaches the supports while leaning into wind.
        const FVector LugMid=(Config.CatchLugPlusM+Config.CatchLugMinusM)*0.5;
        const FQuat CaptureQ=Config.TowerRotation*FQuat(FVector::UpVector,FMath::DegreesToRadians(Config.CaptureHeadingDeg));
        const FVector PoseCorrection=CaptureQ.RotateVector(LugMid)-Body.Rotation.RotateVector(LugMid-FVector(0,0,FlightGeometry::BoosterBaseOffsetM))-FVector(0,0,FlightGeometry::BoosterBaseOffsetM);
        const double PoseBlend=State.Phase==ERecoveryPhase::Capture ? 1-FMath::SmoothStep(30.,130.,State.Navigation.AltitudeM-Config.CaptureWorldM.Z) : 0;
        // Acquire the opening from tower-local +X, then descend between rails.
        // The standoff vanishes while the entire booster is still above the arms.
        const double Standoff=State.Phase==ERecoveryPhase::Capture?0.:18.;
        const FVector Error=Config.CaptureWorldM+Config.TowerRotation.GetForwardVector()*Standoff+FVector(0,0,FlightGeometry::BoosterBaseOffsetM+Clearance-0.25)+PoseCorrection*PoseBlend-Body.OriginM;
        const double Height=FMath::Max(0.,Error.Z*-1);
        const double VerticalGain=FMath::Lerp(0.22,0.7,FMath::Clamp((Height-10)/90.,0.,1.));
        const double TerminalDecel=FMath::Min(Config.LandingDecelerationMps2,0.7*(3*AvailablePerEngine/State.Navigation.MassKg-State.Navigation.Gravity));
        const double DesiredVz=Error.Z>0 ? FMath::Min(3.,Error.Z*0.5) : -FMath::Min(FMath::Sqrt(2*FMath::Max(1.,TerminalDecel)*Height),FMath::Max(State.Phase==ERecoveryPhase::Capture?0.25:0.,Height*VerticalGain));
        FVector Lateral(Error.X,Error.Y,0);
        FVector DesiredVelocity=Lateral*0.12;
        DesiredVelocity=DesiredVelocity.GetClampedToMaxSize(FMath::Max(State.Phase==ERecoveryPhase::LandingBurn ? 40. : 12.,FMath::Abs(State.Navigation.VerticalSpeedMps)*0.35));

        const double VelocityGain=State.Navigation.AltitudeM<200 ? 0.35 : 0.3;
        ForceAccel=(DesiredVelocity-State.Navigation.VelocityMps)*VelocityGain;
        ForceAccel.Z=State.Navigation.Gravity+FMath::Clamp((DesiredVz-State.Navigation.VerticalSpeedMps)*0.8,-4.,13*AvailablePerEngine/State.Navigation.MassKg-State.Navigation.Gravity);
        // Compensate measured modelled aerodynamic force as atmospheric speed falls.
        ForceAccel-=Dynamics.AeroForceN/FMath::Max(1.,State.Navigation.MassKg);
        // Aerodynamic braking can exceed the requested deceleration. The engine
        // cannot thrust downward; retain an upward attitude through that handover.
        ForceAccel.Z=FMath::Max(State.Navigation.Gravity*0.5,ForceAccel.Z);
        // In tail-first flight a tilted body's aerodynamic normal force can
        // exceed the lateral thrust. Solve that coupling instead of repeatedly
        // feeding the previous frame's side force back into the gimbal command.
        const FVector AirVelocity=State.Navigation.VelocityMps-WindAt(State.Navigation.AltitudeM);
        const double AirSpeed=FMath::Max(1.,AirVelocity.Size());
        const double NormalAuthority=State.Navigation.DynamicPressurePa*Config.BodySideAreaM2*Config.BodyNormalCoefficient/State.Navigation.MassKg;
        const double CdA=Config.DragAreaM2*Config.TailFirstDragCoefficient+3*Config.GridFinAreaM2*Config.GridFinDragCoefficient;
        const FVector AxialDrag=-State.Navigation.DynamicPressurePa*CdA*AirVelocity/(AirSpeed*State.Navigation.MassKg);
        const FVector DesiredA=(DesiredVelocity-State.Navigation.VelocityMps)*VelocityGain;
        const double CoupledAuthority=ForceAccel.Z-NormalAuthority;
        FVector Tilt;
        if(FMath::Abs(CoupledAuthority)>ForceAccel.Z*0.2)
            Tilt=FVector(DesiredA.X-AxialDrag.X+NormalAuthority*AirVelocity.X/AirSpeed,DesiredA.Y-AxialDrag.Y+NormalAuthority*AirVelocity.Y/AirSpeed,0)/CoupledAuthority;
        else Tilt=-FVector(AirVelocity.X,AirVelocity.Y,0)/FMath::Max(20.,FMath::Abs(AirVelocity.Z));
        Tilt=Tilt.GetClampedToMaxSize(FMath::Tan(FMath::DegreesToRadians(Config.MaxTiltDeg)));
        // At low speed use the actual force model, including crosswind and fin
        // projected area. The tail-first approximation is only valid in descent.
        if(AirSpeed<50)
            Tilt=FVector(ForceAccel.X,ForceAccel.Y,0).GetClampedToMaxSize(ForceAccel.Z*FMath::Tan(FMath::DegreesToRadians(Config.MaxTiltDeg)))/ForceAccel.Z;
        TargetUp=FVector(Tilt.X,Tilt.Y,1).GetSafeNormal();
        ForceAccel=TargetUp*(ForceAccel.Z/TargetUp.Z);
        const double Required=ForceAccel.Size()*State.Navigation.MassKg;
        // Separate engage/disengage thresholds prevent the guidance command
        // from restarting ten engines on consecutive frames near one boundary.
        // This changes engine requests only; physical valve dynamics still apply.
        const double ThreeEngineDemand=Required/(3*AvailablePerEngine);
        LandingEngineGroup=LandingEngineGroup>=13 ? (ThreeEngineDemand<.80?3:13) : (ThreeEngineDemand>.96?13:3);
        State.Command.EngineCount=LandingEngineGroup;
        if(State.Phase==ERecoveryPhase::Capture)
        {
            State.ArmClosure=FMath::Clamp((60.-Height)/25.,0.,1.);
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
                Transition(ERecoveryPhase::Captured,TEXT("Both fittings resting on rails / engines OFF / free rigid body"));
                State.Command.EngineCount=0;
            }
        }
    }
    State.Command.ThrustAccelerationMps2=ForceAccel;State.Command.TargetUpWorld=TargetUp;
    if(State.Phase==ERecoveryPhase::Coast || State.Phase==ERecoveryPhase::Entry)
    {
        if(Dynamics.ThrustN<1) State.UnpoweredSeconds+=Dt;
        else if(State.PhaseTimeS>2) State.bUnpoweredViolation=true;
    }
    if(Dynamics.PropellantKg<=0 && State.Phase!=ERecoveryPhase::Captured)
    { Transition(ERecoveryPhase::Aborted,TEXT("Main propellant exhausted")); State.Command.EngineCount=0; Fail(State.Events.Last().Message); }
}

void FRecoveryGuidanceModel::PredictBallistic()
{
    // Forward point-mass coast prediction, no engine thrust. Recomputed from measured state.
    FVector P=State.Navigation.BasePositionM,V=State.Navigation.VelocityMps;
    const double Floor=Config.CaptureWorldM.Z+30;
    double T=0;
    for(;T<500 && (P.Z>Floor || V.Z>0);T+=0.75)
    {
        const double H=FlightGeometry::AltitudeM(P*100);
        const auto Air=RecoveryAtmosphere::Sample(H,Config.SeaLevelTemperatureOffsetK);
        const FVector Rel=V-WindAt(H);
        const double Cd=Config.TailFirstDragCoefficient*(1+0.2*FMath::Exp(-FMath::Square((Rel.Size()/Air.SoundSpeed-1)/0.3)));
        const double CdA=Config.DragAreaM2*Cd+3*Config.GridFinAreaM2*Config.GridFinDragCoefficient;
        const FVector A=(FlightGeometry::EarthCenterCm()-P*100).GetSafeNormal()*Air.Gravity-0.5*Air.Density*CdA*Rel.Size()*Rel/FMath::Max(1.,State.Navigation.MassKg);
        P+=V*0.75+A*(0.5*0.75*0.75); V+=A*0.75;
    }
    State.TimeToImpactS=FMath::Max(1.,T);
    State.PredictedImpactM=P+FVector(V.X,V.Y,0)*Config.LandingDriftCorrectionS;
    State.PredictedMissM=FVector2D(State.PredictedImpactM-Config.CaptureWorldM).Size();
}

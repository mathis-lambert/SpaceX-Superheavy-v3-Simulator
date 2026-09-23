#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::Guide(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,double Dt)
{
    PredictorClock+=Dt;
    if(PredictorClock>=0.25) { PredictorClock-=0.25; PredictBallistic(); }
    if(State.bAlternateRecovery)
    {
        FVector Force,Up;GuideAlternate(Dynamics,Dt,Force,Up);
        State.Command.ThrustAccelerationMps2=Force;State.Command.TargetUpWorld=Up;return;
    }
    const FVector Downrange=Config.TowerRotation.GetForwardVector();
    const FVector SiteWind=WindAt(Config.ReturnWindReferenceAltitudeM);
    FVector Crosswind=SiteWind-Downrange*FVector::DotProduct(SiteWind,Downrange);Crosswind.Z=0;
    // The coast predictor has no powered-braking model. Reserve front distance
    // and an estimated upwind offset before the final force-constrained transfer.
    const FVector ReturnTarget=Config.CaptureWorldM+Downrange*Config.FrontReturnOffsetM-Crosswind*Config.LandingWindLeadS;
    const double AvailablePerEngine=Config.EngineThrustN*State.Navigation.EngineIspS/Config.SpecificImpulseSeaLevelS;
    FVector ForceAccel=FVector::ZeroVector,TargetUp=FVector::UpVector;
    State.Command.EngineCount=0;
    State.TargetPositionM=Config.CaptureWorldM;
    if(State.Phase>=ERecoveryPhase::Separation && State.Phase<=ERecoveryPhase::Entry)State.TargetPositionM=ReturnTarget;
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
            Transition(ERecoveryPhase::Separation,ERecoveryGuidanceReason::Separation);
        }
    }
    if(State.Phase==ERecoveryPhase::Separation || State.Phase==ERecoveryPhase::Boostback)
    {
        const double Tgo=FMath::Max(40.,State.TimeToImpactS);
        FVector DV=(ReturnTarget-State.PredictedImpactM)*(1.18/Tgo); DV.Z=0;
        const double ApogeeError=Config.ApogeeM-State.Navigation.AltitudeM;
        const double DesiredVz=FMath::Sign(ApogeeError)*FMath::Sqrt(2*State.Navigation.Gravity*FMath::Abs(ApogeeError))*0.75;
        ForceAccel=DV/6.; ForceAccel.Z=(DesiredVz-State.Navigation.VerticalSpeedMps)/8.+State.Navigation.Gravity;
        TargetUp=ForceAccel.GetSafeNormal();
        State.Command.EngineCount=State.Phase==ERecoveryPhase::Boostback ? 13 : 0;
        if(State.Phase==ERecoveryPhase::Separation && External.bSeparated && State.PhaseTimeS>2 && FVector::DotProduct(TargetUp,Body.Rotation.GetUpVector())>0.97)
        {
            State.BoostbackIgnitionAltitudeM=State.Navigation.AltitudeM; State.BoostbackDownrangeM=State.Navigation.HorizontalErrorM;
            Transition(ERecoveryPhase::Boostback,ERecoveryGuidanceReason::Boostback); State.Command.EngineCount=13;
        }
        if(State.Phase==ERecoveryPhase::Boostback)
        {
            State.BoostbackSeconds+=Dt;
            const double PredictedApogee=State.Navigation.AltitudeM+FMath::Square(FMath::Max(0.,State.Navigation.VerticalSpeedMps))/(2*State.Navigation.Gravity);
            if(State.PhaseTimeS>5 && FVector2D(State.PredictedImpactM-ReturnTarget).Size()<500 && PredictedApogee<Config.ApogeeM+3000)
            { Transition(ERecoveryPhase::Coast,ERecoveryGuidanceReason::Coast); State.Command.EngineCount=0; }
            else if(Dynamics.PropellantKg<Config.LandingReserveKg)
            { SelectAlternate(Dynamics);State.Command.EngineCount=0;ForceAccel=FVector::ZeroVector; }
        }
    }
    if(State.Phase==ERecoveryPhase::Coast && State.Navigation.VerticalSpeedMps<0 && State.Navigation.DynamicPressurePa>200)
        Transition(ERecoveryPhase::Entry,ERecoveryGuidanceReason::Entry);
    if(!State.bAlternateRecovery && (State.Phase==ERecoveryPhase::Coast || State.Phase==ERecoveryPhase::Entry))
    {
        State.Command.EngineCount=0; ForceAccel=FVector::ZeroVector;
        // Tail-first attitude and aerodynamic correction of the predicted entry point.
        const FVector Rel=State.Navigation.VelocityMps-WindAt(State.Navigation.AltitudeM);
        TargetUp=Rel.Z<-30 ? -Rel.GetSafeNormal() : FVector::UpVector;
        if(State.Phase==ERecoveryPhase::Entry)
        {
            const double LookAhead=FMath::Max(8.,State.TimeToImpactS);
            FVector DesiredA=(ReturnTarget-State.PredictedImpactM)*(2./(LookAhead*LookAhead)); DesiredA.Z=0;
            const double Authority=State.Navigation.DynamicPressurePa*Config.BodySideAreaM2*Config.BodyNormalCoefficient/FMath::Max(1.,State.Navigation.MassKg);
            FVector Correction=(-DesiredA/FMath::Max(0.1,Authority)).GetClampedToMaxSize(FMath::Tan(FMath::DegreesToRadians(Config.MaxEntryAngleDeg)));
            TargetUp=(TargetUp+Correction).GetSafeNormal();
        }
        // A displaced entry footprint can require propulsion before terminal
        // braking. Hysteresis prevents valve chatter while the prediction updates.
        const double Miss=FVector2D(ReturnTarget-State.PredictedImpactM).Size();
        const double Remaining=FMath::Max(0.,Dynamics.PropellantKg-Config.LandingReserveKg);
        State.CorrectionFuelBudgetKg=Remaining;
        const double Reserve=FMath::Max(Config.LandingReserveKg,State.LandingPrediction.FuelKg);
        const double Spare=FMath::Max(0.,Dynamics.PropellantKg-Reserve);
        const double Budget=State.Navigation.EngineIspS*RecoveryAtmosphere::G0*
            FMath::Loge(State.Navigation.MassKg/FMath::Max(1.,State.Navigation.MassKg-Spare));
        if(State.Phase==ERecoveryPhase::Entry && State.Navigation.AltitudeM>12000 && Miss>10000 &&
            Miss/FMath::Max(8.,State.TimeToImpactS)>Budget*.7)
        {
            SelectAlternate(Dynamics);GuideAlternate(Dynamics,Dt,ForceAccel,TargetUp);
            State.Command.ThrustAccelerationMps2=ForceAccel;State.Command.TargetUpWorld=TargetUp;return;
        }
        if(!State.bCorrectiveBurn && Miss>5000 && State.Navigation.AltitudeM>18000 &&
            State.Navigation.DynamicPressurePa<1200 && Remaining>2500)State.bCorrectiveBurn=true;
        if(Miss<900 || Remaining<500 || State.Navigation.AltitudeM<12000 || State.Navigation.DynamicPressurePa>2500)State.bCorrectiveBurn=false;
        if(State.bCorrectiveBurn)
        {
            FVector DeltaV=(ReturnTarget-State.PredictedImpactM)/FMath::Max(20.,State.TimeToImpactS);DeltaV.Z=0;
            const double Accel=3*AvailablePerEngine/State.Navigation.MassKg;
            TargetUp=DeltaV.GetSafeNormal();
            // Turn using existing fins/RCS, then fire only along a useful axis.
            if(FVector::DotProduct(TargetUp,Body.Rotation.GetUpVector())>.94)
            {State.Command.EngineCount=3;ForceAccel=TargetUp*FMath::Min(Accel,DeltaV.Size()/4.);State.CorrectiveBurnSeconds+=Dt;State.LastCorrectionBurnTimeS=State.SampleTimeS;}
        }
        LandingPredictionClock-=Dt;
        if(State.Navigation.VerticalSpeedMps<-20 && State.Navigation.AltitudeM<10000 && LandingPredictionClock<=0)
        {
            LandingPredictionClock=.1;
            State.LandingPrediction=RecoveryLanding::Predict(Config,Dynamics,State.Navigation.AltitudeM,
                State.Navigation.VerticalSpeedMps,Body.Rotation.GetUpVector().Z,Config.LandingDecelerationMps2,External.Experiment.FailedEngine);
        }
        // The extra control interval covers the sampled decision latency. The
        // prediction itself includes the physical engine opening response.
        const double TriggerDistance=State.LandingPrediction.DistanceM+Config.LandingBurnMarginM+
            FMath::Max(0.,-State.Navigation.VerticalSpeedMps)*.1;
        if(State.Navigation.VerticalSpeedMps<-20 && State.LandingPrediction.bFeasible &&
            State.Navigation.AltitudeM-Config.CaptureWorldM.Z<=TriggerDistance)
        {
            State.LandingIgnitionAltitudeM=State.Navigation.AltitudeM;
            State.LandingIgnitionTimeS=State.SampleTimeS;
            State.LandingIgnitionSpeedMps=-State.Navigation.VerticalSpeedMps;
            State.LandingIgnitionMassKg=State.Navigation.MassKg;
            State.LandingIgnitionDistanceM=State.LandingPrediction.DistanceM;
            State.LandingIgnitionFuelKg=State.LandingPrediction.FuelKg;
            State.LandingIgnitionThrustN=State.LandingPrediction.LandingThrustN;
            Transition(ERecoveryPhase::LandingBurn,ERecoveryGuidanceReason::LandingBurn);
        }
    }
    if(State.Phase==ERecoveryPhase::LandingBurn || State.Phase==ERecoveryPhase::Capture)
        GuideLanding(Dynamics,External,Dt,ForceAccel,TargetUp);
    State.Command.ThrustAccelerationMps2=ForceAccel;State.Command.TargetUpWorld=TargetUp;
    if(State.Phase==ERecoveryPhase::Coast || State.Phase==ERecoveryPhase::Entry)
    {
        if(Dynamics.ThrustN<1) State.UnpoweredSeconds+=Dt;
        else if(State.PhaseTimeS>2 && !State.bCorrectiveBurn && State.SampleTimeS-State.LastCorrectionBurnTimeS>2) State.bUnpoweredViolation=true;
    }
    if(Dynamics.PropellantKg<=0 && State.Phase!=ERecoveryPhase::Captured)
    { State.Command.EngineCount=0;SelectAlternate(Dynamics); }
}

void FRecoveryGuidanceModel::PredictBallistic()
{
    // Forward point-mass coast prediction, no engine thrust. Recomputed from measured state.
    FVector P=State.Navigation.BasePositionM,V=State.Navigation.VelocityMps;
    State.BallisticPathM.Reset();State.BallisticPathM.Add(P);
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
        if(FMath::Fmod(T,3.)<.1)State.BallisticPathM.Add(P);
    }
    State.TimeToImpactS=FMath::Max(1.,T);
    State.PredictedImpactM=P+FVector(V.X,V.Y,0)*Config.LandingDriftCorrectionS;
    State.PredictedMissM=FVector2D(State.PredictedImpactM-Config.CaptureWorldM).Size();
}

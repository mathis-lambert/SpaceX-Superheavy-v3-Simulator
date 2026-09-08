#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Flight/RecoveryApproachGeometry.h"

void FRecoveryGuidanceModel::Reset(const FRecoveryGuidanceConfiguration& Configuration)
{
    Config=Configuration;State=FRecoveryGuidanceState();State.Events.Reserve(16);
    PredictorClock=0;TerminalClock=0;LandingEngineGroup=13;
}
void FRecoveryGuidanceModel::Transition(ERecoveryPhase Phase,ERecoveryGuidanceReason Reason)
{
    State.Phase=Phase;State.PhaseTimeS=0;
    State.Events.Add({Phase,Reason,State.MissionTimeS,State.SampleTimeS,State.Navigation.AltitudeM,State.Navigation.MassKg});
}
void FRecoveryGuidanceModel::Fail(ERecoveryGuidanceReason Reason)
{
    if(State.Phase!=ERecoveryPhase::Aborted)Transition(ERecoveryPhase::Aborted,Reason);
    State.ResultReason=Reason;State.bResultReady=true;State.bSuccess=false;
}
FVector FRecoveryGuidanceModel::WindAt(double Height) const
{
    return RecoveryAtmosphere::WindAt(Config.WindVelocityMps,Height,Experiment.WindScale);
}
void FRecoveryGuidanceModel::Navigate(const FRecoveryDynamicsState& Dynamics,bool Separated)
{
    State.Navigation=RecoveryNavigation::Evaluate(Config,Body,Dynamics.PropellantKg,Dynamics.RcsPropellantKg,Separated,Experiment);
}

void FRecoveryGuidanceModel::AuditFrontApproach()
{
    if(State.Phase<ERecoveryPhase::Separation || State.Phase>ERecoveryPhase::Captured)return;
    const auto& N=State.Navigation;
    State.MinimumMastFrontMarginM=FMath::Min(State.MinimumMastFrontMarginM,
        RecoveryApproach::MastFrontMargin(N.BasePositionM,Body.Rotation.GetUpVector(),Config.TowerWorldM,Config.TowerRotation));
    if(N.AltitudeM-Config.CaptureWorldM.Z>=RecoveryApproach::FinalApproachHeightM)return;
    ++State.FrontApproachSamples;
    const FVector LugMid=(Config.CatchLugPlusM+Config.CatchLugMinusM)*.5;
    const FQuat CaptureQ=Config.TowerRotation*FQuat(FVector::UpVector,FMath::DegreesToRadians(Config.CaptureHeadingDeg));
    const FVector Fitting=N.BasePositionM+Body.Rotation.RotateVector(LugMid);
    const FVector Target=Config.CaptureWorldM+CaptureQ.RotateVector(LugMid);
    State.MinimumFrontCorridorMarginM=FMath::Min(State.MinimumFrontCorridorMarginM,
        RecoveryApproach::CorridorMargin(Fitting,Target,Config.TowerRotation));
    const FVector Local=Config.TowerRotation.UnrotateVector(Fitting-Target);
    const FVector V=Config.TowerRotation.UnrotateVector(N.VelocityMps);
    if(Local.X>40 && Local.X<250 && RecoveryApproach::CorridorMargin(Fitting,Target,Config.TowerRotation)>=0 &&
        V.X<-.1 && N.HeadingErrorDeg<5.)State.bFrontIngressVerified=true;
}

void FRecoveryGuidanceModel::Step(const FRecoveryBodyKinematics& Kinematics,const FRecoveryDynamicsState& Dynamics,
    const FRecoveryDynamicsCommand& External,double Dt)
{
    if(Dt<=0 || !FMath::IsFinite(Dt))return;
    Body=Kinematics;Experiment=External.Experiment;
    State.Command=External;
    Navigate(Dynamics,External.bSeparated);
    // Ground operations and explicit collision fixtures have separate command
    // ownership. A flight begins only after the real launch mount is released.
    if(External.bExternalFlightFixture || External.Phase<ERecoveryPhase::Ascent)
    {State.Phase=External.Phase;return;}
    if(!State.bFlightStarted)
    {
        if(External.Phase!=ERecoveryPhase::Ascent){State.Phase=External.Phase;return;}
        State.bFlightStarted=true;State.Phase=ERecoveryPhase::Ascent;
    }
    ++State.Steps;
    State.ElapsedS+=Dt;State.MinimumStepS=FMath::Min(State.MinimumStepS,Dt);State.MaximumStepS=FMath::Max(State.MaximumStepS,Dt);
    // Navigation reads the incoming body sample. The command covers the next
    // interval and the mission clock below records that interval's endpoint.
    State.SampleTimeS=State.MissionTimeS;
    if(!State.bResultReady)State.MissionTimeS+=Dt;
    State.PhaseTimeS+=Dt;
    State.Command.EngineCount=0;State.Command.ThrustAccelerationMps2=FVector::ZeroVector;
    State.Command.TargetUpWorld=FVector::UpVector;
    State.Command.TargetAngularVelocityWorldRadS=FVector::ZeroVector;
    if(External.Phase==ERecoveryPhase::Aborted && State.Phase!=ERecoveryPhase::Aborted)
        Fail(ERecoveryGuidanceReason::OperatorAbort);
    if(State.Phase>=ERecoveryPhase::Ascent && State.Phase<=ERecoveryPhase::Capture)
    {
        if(State.MissionTimeS>Config.TimeoutSeconds || (State.Phase>=ERecoveryPhase::LandingBurn && State.Navigation.TiltDeg>70) ||
            State.Navigation.AltitudeM < -3 || Body.OriginM.ContainsNaN())Fail(ERecoveryGuidanceReason::EnvelopeExceeded);
        else Guide(Dynamics,External,Dt);
    }
    AuditFrontApproach();
    if(State.Phase==ERecoveryPhase::Captured)
    {
        if((State.Navigation.BasePositionM-State.LatchPositionM).Size()>1. || State.Navigation.TiltDeg>5.)
            Fail(ERecoveryGuidanceReason::SupportLost);
        else if(State.PhaseTimeS>8 && !State.bResultReady)
        {
            State.bResultReady=true;State.bSuccess=State.Navigation.VelocityMps.Size()<.15 && External.SupportContactCount==2;
            State.ResultReason=ERecoveryGuidanceReason::SupportEvaluated;
            if(!State.bFrontIngressVerified || State.MinimumMastFrontMarginM<0 || State.MinimumFrontCorridorMarginM<0)
            {
                State.bSuccess=false;
                State.ResultReason=ERecoveryGuidanceReason::ApproachEnvelopeExceeded;
            }
        }
    }
    State.Command.Phase=State.Phase;State.Command.bContactShutdown=State.bContactShutdown;
    if(State.Phase>=ERecoveryPhase::Captured)
    {State.Command.EngineCount=0;State.Command.ThrustAccelerationMps2=FVector::ZeroVector;}
    State.Command.DelugeDemand=(State.Phase==ERecoveryPhase::Ascent && State.Navigation.AltitudeM<140) ||
        (State.Phase>=ERecoveryPhase::LandingBurn && State.Phase<=ERecoveryPhase::Capture && State.Navigation.AltitudeM<220)?1.:0.;
    if(State.Phase==ERecoveryPhase::Aborted)State.Command.DelugeDemand=External.DelugeDemand;
}

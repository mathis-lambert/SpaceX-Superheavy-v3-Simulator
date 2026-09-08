#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryGuidanceModel::Reset(const FRecoveryGuidanceConfiguration& Configuration)
{
    Config=Configuration;State=FRecoveryGuidanceState();State.Events.Reserve(16);
    PredictorClock=0;LandingEngineGroup=13;
}
void FRecoveryGuidanceModel::Transition(ERecoveryPhase Phase,const FString& Message)
{
    State.Phase=Phase;State.PhaseTimeS=0;
    State.Events.Add({Phase,State.MissionTimeS,Message});
}
void FRecoveryGuidanceModel::Fail(const FString& Reason)
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
    if(!State.bResultReady)State.MissionTimeS+=Dt;
    State.PhaseTimeS+=Dt;
    State.Command.EngineCount=0;State.Command.ThrustAccelerationMps2=FVector::ZeroVector;
    State.Command.TargetUpWorld=FVector::UpVector;
    if(External.Phase==ERecoveryPhase::Aborted && State.Phase!=ERecoveryPhase::Aborted)
        Fail(TEXT("Operator abort / engines shut down"));
    if(State.Phase>=ERecoveryPhase::Ascent && State.Phase<=ERecoveryPhase::Capture)
    {
        if(State.MissionTimeS>Config.TimeoutSeconds || (State.Phase>=ERecoveryPhase::LandingBurn && State.Navigation.TiltDeg>70) ||
            State.Navigation.AltitudeM < -3 || Body.OriginM.ContainsNaN())Fail(TEXT("Flight envelope exceeded"));
        else Guide(Dynamics,External,Dt);
    }
    if(State.Phase==ERecoveryPhase::Captured)
    {
        if((State.Navigation.BasePositionM-State.LatchPositionM).Size()>1. || State.Navigation.TiltDeg>5.)
            Fail(TEXT("Physical support lost after engine shutdown"));
        else if(State.PhaseTimeS>8 && !State.bResultReady)
        {
            State.bResultReady=true;State.bSuccess=State.Navigation.VelocityMps.Size()<.15 && External.SupportContactCount==2;
            State.ResultReason=TEXT("Physical rail support evaluated for eight seconds with engines off");
        }
    }
    State.Command.Phase=State.Phase;State.Command.bContactShutdown=State.bContactShutdown;
    if(State.Phase>=ERecoveryPhase::Captured)
    {State.Command.EngineCount=0;State.Command.ThrustAccelerationMps2=FVector::ZeroVector;}
    State.Command.DelugeDemand=(State.Phase==ERecoveryPhase::Ascent && State.Navigation.AltitudeM<140) ||
        (State.Phase>=ERecoveryPhase::LandingBurn && State.Phase<=ERecoveryPhase::Capture && State.Navigation.AltitudeM<220)?1.:0.;
    if(State.Phase==ERecoveryPhase::Aborted)State.Command.DelugeDemand=External.DelugeDemand;
}

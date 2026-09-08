#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::TickLaunchSequence(double Dt)
{
    const bool SystemsReady=Experiment.FailedEngine<0 && Experiment.JammedFin<0 &&
        !Experiment.bReactionJetsDisabled && PropellantKg>RuntimeProfile->LandingReserveKg;
    bool ThrustVerified=Engines.Num()==33 && ActualThrustN>MassKg*Gravity*1.1;
    for(const auto& Engine:Engines)
        ThrustVerified&=Engine.ThrustN>RuntimeProfile->EngineThrustN*.9;
    const FString Previous=LaunchSequence.Label();
    LaunchSequence.Advance(Dt,SystemsReady,ThrustVerified);
    MissionTime=-LaunchSequence.RemainingS;
    StatusMessage=LaunchSequence.Label();
    if(Previous!=StatusMessage)
    {
        PhaseEvents.Add(FString::Printf(TEXT("GROUND %.3fs %s"),MissionTime,*StatusMessage));
        UE_LOG(LogTemp,Display,TEXT("GROUND_SEQUENCE t=%.3f event=%s"),MissionTime,*StatusMessage);
    }
    if(LaunchSequence.bAborted)
    {
        SetPhase(ERecoveryPhase::Aborted,TEXT("Launch interlock failed / mount retained"));
        ActiveEngines=0;
        WriteResult(false,StatusMessage);
    }
    else if(LaunchSequence.bReleaseRequested)
    {
        ReleaseLaunchHoldDown();MissionTime=0;
        SetPhase(ERecoveryPhase::Ascent,TEXT("Mount released / thrust verified"));
    }
}

void ASuperHeavyRecoveryDirector::PrepareGroundCommand()
{
    DynamicsCommand.bGroundSupplyConnected=!bLaunchHoldReleased && LaunchSequence.IsGroundSupplyConnected();
    DynamicsCommand.DelugeDemand=Phase==ERecoveryPhase::Countdown || Phase==ERecoveryPhase::Aborted ? LaunchSequence.DelugeDemand() :
        (Phase==ERecoveryPhase::Ascent && AltitudeM<140) || (Phase>=ERecoveryPhase::LandingBurn && Phase<=ERecoveryPhase::Capture && AltitudeM<220) ? 1. : 0.;
}

#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"

void ASuperHeavyRecoveryDirector::ConsumeGuidanceState()
{
    const auto& G=GuidanceState;
    if(!G.bFlightStarted)return;
    // Events contain the physical decision time. Ground-thread acknowledgement
    // of a separation request creates the second body; boostback waits for it.
    for(;ConsumedGuidanceEvents<G.Events.Num();++ConsumedGuidanceEvents)
    {
        const auto& Event=G.Events[ConsumedGuidanceEvents];
        MissionTime=Event.TimeS;SetPhase(Event.Phase,Event.Message);
    }
    MissionTime=G.MissionTimeS;PhaseTime=G.PhaseTimeS;
    UpdateNavigation();
    if(G.bSeparationRequested && !bSeparated)SeparateUpperStage();
    ActiveEngines=Phase==ERecoveryPhase::Aborted?0:G.Command.EngineCount;
    bContactShutdown=G.bContactShutdown;
    TargetPositionM=G.TargetPositionM;PredictedImpactM=G.PredictedImpactM;
    TimeToImpactS=G.TimeToImpactS;PredictedMissM=G.PredictedMissM;
    BoostbackIgnitionAltitudeM=G.BoostbackIgnitionAltitudeM;BoostbackDownrangeM=G.BoostbackDownrangeM;BoostbackSeconds=G.BoostbackSeconds;
    LandingIgnitionAltitudeM=G.LandingIgnitionAltitudeM;LandingBurnSeconds=G.LandingBurnSeconds;
    UnpoweredSeconds=G.UnpoweredSeconds;bUnpoweredViolation=G.bUnpoweredViolation;
    CaptureDwell=G.SettledContactSeconds;CaptureErrorAtLatch=G.CaptureErrorAtLatch;
    CaptureSpeedAtLatch=G.CaptureSpeedAtLatch;CaptureTiltAtLatch=G.CaptureTiltAtLatch;
    CaptureHeadingAtLatch=G.CaptureHeadingAtLatch;CaptureLugAtLatch=G.CaptureLugAtLatch;LatchPositionM=G.LatchPositionM;
    Tower->SetArmClosure(G.ArmClosure);
    if(G.bResultReady && !bResultWritten)WriteResult(G.bSuccess,G.ResultReason);
}

#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"

const TCHAR* RecoveryGuidanceReasonText(ERecoveryGuidanceReason Reason)
{
    switch(Reason)
    {
    case ERecoveryGuidanceReason::Splashdown:return TEXT("Splashdown / engines off / floating hull");
    case ERecoveryGuidanceReason::AlternateSelected:return TEXT("Tower recovery abandoned / offshore diversion");
    case ERecoveryGuidanceReason::EmergencyContact:return TEXT("Emergency surface contact / tower mission not recovered");
    case ERecoveryGuidanceReason::ImpactMitigation:return TEXT("No reachable offshore candidate / braking to reduce impact");
    case ERecoveryGuidanceReason::Separation:return TEXT("MECO / stage separation / return attitude");
    case ERecoveryGuidanceReason::Boostback:return TEXT("13-engine boostback / solving ballistic return");
    case ERecoveryGuidanceReason::Coast:return TEXT("Boostback cutoff / unpowered coast to apogee");
    case ERecoveryGuidanceReason::ReserveDepleted:return TEXT("Boostback depleted landing reserve");
    case ERecoveryGuidanceReason::Entry:return TEXT("Engines OFF / atmospheric descent / grid-fin guidance");
    case ERecoveryGuidanceReason::LandingBurn:return TEXT("Landing burn / 13 to 3 Raptor engines");
    case ERecoveryGuidanceReason::Capture:return TEXT("Final descent / catch-fittings and heading alignment");
    case ERecoveryGuidanceReason::Captured:return TEXT("Both fittings resting on rails / engines OFF / free rigid body");
    case ERecoveryGuidanceReason::PropellantExhausted:return TEXT("Main propellant exhausted");
    case ERecoveryGuidanceReason::OperatorAbort:return TEXT("Operator abort / engines shut down");
    case ERecoveryGuidanceReason::EnvelopeExceeded:return TEXT("Flight envelope exceeded");
    case ERecoveryGuidanceReason::SupportLost:return TEXT("Physical support lost after engine shutdown");
    case ERecoveryGuidanceReason::SupportEvaluated:return TEXT("Physical rail support evaluated for eight seconds with engines off");
    case ERecoveryGuidanceReason::ApproachEnvelopeExceeded:return TEXT("Front approach corridor violated before capture");
    default:return TEXT("");
    }
}

void ASuperHeavyRecoveryDirector::ConsumeGuidanceState()
{
    const auto& G=GuidanceState;
    if(!G.bFlightStarted)return;
    // Events contain the physical decision time. Ground-thread acknowledgement
    // of a separation request creates the second body; boostback waits for it.
    for(;ConsumedGuidanceEvents<G.Events.Num();++ConsumedGuidanceEvents)
    {
        const auto& Event=G.Events[ConsumedGuidanceEvents];
        RecordPhase(Event.Phase,RecoveryGuidanceReasonText(Event.Reason),Event.TimeS,Event.AltitudeM,Event.MassKg);
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
    if(G.bResultReady && !bResultWritten)WriteResult(G.bSuccess,RecoveryGuidanceReasonText(G.ResultReason));
}

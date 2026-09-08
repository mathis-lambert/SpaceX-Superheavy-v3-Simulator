#include "Recovery/Flight/RecoveryFlightInspection.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::ApplyVehicleForce(ERecoveryForceKind Kind,int32 Index,const FVector& ForceN,const FVector& PointCm)
{
    Body->AddForceAtLocation(ForceN*100.,PointCm);
    AppliedForces.Add({Kind,Index,PointCm,ForceN});
}
void ASuperHeavyRecoveryDirector::RecordExperiment(const TCHAR* Action)
{
    const FString Event=FString::Printf(TEXT("EXPERIMENT %s t=%.3f engine=%d fin=%d rcs_off=%d response=%.3f wind=%.3f"),Action,MissionTime,
        Experiment.FailedEngine,Experiment.JammedFin,Experiment.bReactionJetsDisabled,Experiment.AttitudeResponse,Experiment.WindScale);
    PhaseEvents.Add(Event);
    UE_LOG(LogTemp,Display,TEXT("%s"),*Event);
}
void ASuperHeavyRecoveryDirector::SetFailedEngine(int32 Index)
{ Experiment.FailedEngine=Engines.IsValidIndex(Index)?Index:INDEX_NONE;RecordExperiment(TEXT("ENGINE")); }
void ASuperHeavyRecoveryDirector::SetJammedFin(int32 Index)
{
    Experiment.JammedFin=Index>=0 && Index<3?Index:INDEX_NONE;
    if(Experiment.JammedFin>=0)Experiment.JammedFinAngleDeg=GridFinAnglesDeg[Experiment.JammedFin];
    RecordExperiment(TEXT("FIN_JAM"));
}
void ASuperHeavyRecoveryDirector::SetReactionJetsDisabled(bool Disabled)
{ Experiment.bReactionJetsDisabled=Disabled;RecordExperiment(TEXT("RCS")); }
void ASuperHeavyRecoveryDirector::SetAttitudeResponse(double Value)
{ if(FMath::IsFinite(Value))Experiment.AttitudeResponse=FMath::Clamp(Value,.5,1.5);RecordExperiment(TEXT("RESPONSE")); }
void ASuperHeavyRecoveryDirector::SetWindScale(double Value)
{ if(FMath::IsFinite(Value))Experiment.WindScale=FMath::Clamp(Value,0.,3.);RecordExperiment(TEXT("WIND")); }
void ASuperHeavyRecoveryDirector::ResetExperiments()
{ Experiment=FRecoveryFlightExperiment();RecordExperiment(TEXT("RESTORE")); }

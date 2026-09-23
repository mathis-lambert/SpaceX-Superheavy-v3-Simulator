#include "Recovery/Flight/RecoveryFlightInspection.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::RecordExperiment(const TCHAR* Action)
{
    const FString Event=FString::Printf(TEXT("EXPERIMENT %s t=%.3f engine=%d fin=%d rcs_off=%d response=%.3f wind=%.3f"),Action,MissionTime,
        Experiment.FailedEngine,Experiment.JammedFin,Experiment.bReactionJetsDisabled,Experiment.AttitudeResponse,Experiment.WindScale);
    PhaseEvents.Add(Event);
    UE_LOG(LogTemp,Display,TEXT("%s"),*Event);
}
void ASuperHeavyRecoveryDirector::SetFailedEngine(int32 Index)
{ Experiment.EngineRestoreTimeS=-1;Experiment.FailedEngine=Engines.IsValidIndex(Index)?Index:INDEX_NONE;RecordExperiment(TEXT("ENGINE")); }
void ASuperHeavyRecoveryDirector::SetJammedFin(int32 Index)
{
    Experiment.FinRestoreTimeS=-1;
    Experiment.JammedFin=Index>=0 && Index<3?Index:INDEX_NONE;
    if(Experiment.JammedFin>=0)Experiment.JammedFinAngleDeg=GridFinAnglesDeg[Experiment.JammedFin];
    RecordExperiment(TEXT("FIN_JAM"));
}
void ASuperHeavyRecoveryDirector::SetReactionJetsDisabled(bool Disabled)
{ Experiment.RcsRestoreTimeS=-1;Experiment.bReactionJetsDisabled=Disabled;RecordExperiment(TEXT("RCS")); }
void ASuperHeavyRecoveryDirector::SetAttitudeResponse(double Value)
{ if(FMath::IsFinite(Value))Experiment.AttitudeResponse=FMath::Clamp(Value,.5,1.5);RecordExperiment(TEXT("RESPONSE")); }
void ASuperHeavyRecoveryDirector::SetWindScale(double Value)
{ if(FMath::IsFinite(Value))Experiment.WindScale=FMath::Clamp(Value,0.,3.);RecordExperiment(TEXT("WIND")); }
void ASuperHeavyRecoveryDirector::ResetExperiments()
{ Experiment=FRecoveryFlightExperiment();RecordExperiment(TEXT("RESTORE")); }

void ASuperHeavyRecoveryDirector::SetTimedFault(int32 Kind,int32 Index,double DurationS)
{
    if(!FMath::IsFinite(DurationS) || DurationS<=0)return;
    const double Deadline=FMath::Max(0.,MissionTime)+FMath::Clamp(DurationS,.05,60.);
    if(Kind==1){SetFailedEngine(Index);Experiment.EngineRestoreTimeS=Deadline;}
    else if(Kind==2){SetJammedFin(Index);Experiment.FinRestoreTimeS=Deadline;}
    else if(Kind==3){SetReactionJetsDisabled(true);Experiment.RcsRestoreTimeS=Deadline;}
    else return;
    PhaseEvents.Add(FString::Printf(TEXT("TIMED_FAULT kind=%d index=%d start=%.6f restore=%.6f"),Kind,Index,MissionTime,Deadline));
}

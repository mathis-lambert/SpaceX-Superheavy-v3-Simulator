#include "Recovery/Diagnostics/RecoveryDiagnosticsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(Recovery,true);

void URecoveryDiagnosticsComponent::RecordPerformanceFrame()
{
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D)return;
    // Correlate the flight with engine GPU, streaming, PSO and RHI counters.
    // No actor search or component inventory in this per-frame measurement.
    CSV_CUSTOM_STAT(Recovery,Phase,int32(D->Phase),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(Recovery,MissionTimeS,float(D->MissionTime),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(Recovery,AltitudeM,float(D->AltitudeM),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(Recovery,Camera,D->Viewer->GetCameraMode(),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(Recovery,ActiveEngines,D->ActiveEngines,ECsvCustomStatOp::Set);
    if(LastPerformancePhase!=int32(D->Phase))
    {
        CSV_EVENT(Recovery,TEXT("PHASE %s T=%.3f"),*D->GetPhaseLabel(),D->MissionTime);
        LastPerformancePhase=int32(D->Phase);
    }
#if CSV_PROFILER
    // Let CSV finish writing before -ExitAfterCsvProfiling closes the process.
    // The fixed frame count remains a cap for a flight that never finishes.
    if(bEndProfileOnResult && !bProfileStopRequested && D->HasMissionResult() && FCsvProfiler::IsCapturing())
    {bProfileStopRequested=true;FCsvProfiler::Get()->EndCapture();}
#endif
}

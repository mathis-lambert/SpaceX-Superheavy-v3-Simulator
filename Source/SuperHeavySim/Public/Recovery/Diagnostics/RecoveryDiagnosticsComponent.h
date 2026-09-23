#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryDiagnosticsComponent.generated.h"

class FJsonValue;

/** Opt-in measurements taken after the engine has cached the current camera POV. */
UCLASS()
class URecoveryDiagnosticsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryDiagnosticsComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void TickVisualReview();
    void WriteVisualReview();
    bool bVisualReview=false,bCloudReview=false;
    bool bWeakReactionReviewed=false;
    FString VisualReviewDirectory;
    double PreviousReviewTime=-DBL_MAX;
    int32 LastReviewPhase=-1;
    TArray<TSharedPtr<FJsonValue>> ReviewFrames;
    void RecordPerformanceFrame();
    bool bPerformanceAudit=false;
    bool bEndProfileOnResult=false,bProfileStopRequested=false;
    int32 LastPerformancePhase=-1;
    void TickGroundAudit(float Dt);
    bool bGroundAudit=false,bGroundPassed=true;
    int32 GroundStage=0;
    double GroundAuditClock=0,GroundHoldTime=0,GroundWallStartS=0;
    uint32 GroundGeneration=0;
    TArray<FString> GroundChecks;
    int64 Samples=0,HighAltitudeSamples=0;
    double LastWallFrame=0,MaxAcousticDelayS=0;
    int FirstLaunchPSOPeak=0;
    TArray<double> FirstLaunchFrameMs;
    int32 PeakVolumes=0,PlumeLights=0,SiteLights=0,PlayingAudio=0;
    int32 PeakTurbulentVolumes=0;
    int32 StarshipFrames=0,StarshipPlumes=0,SiteDetailInstances=0;
    double MaxStarshipErrorCm=0;
    int32 ChaseContactFrames=0;
    FVector LastChaseOffset=FVector::ZeroVector;
    FQuat LastChaseRotation=FQuat::Identity;
    double MaxChaseContactStepCm=0,MaxChaseContactAngleDeg=0;
    double PeakAltitude=0,PeakSpeed=0,MaxCameraErrorCm=0,MaxAngleErrorDeg=0;
    bool bEnabled=false,bCaptured=false,bVaporLit=false,bVaporHasDensity=false;
    bool bRecordAudio=false,bAudioRecording=false,bAudioRecorded=false;
};

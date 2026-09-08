#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryDiagnosticsComponent.generated.h"

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
    void TickGroundAudit(float Dt);
    bool bGroundAudit=false,bGroundPassed=true;
    int32 GroundStage=0;
    double GroundAuditClock=0,GroundHoldTime=0;
    uint32 GroundGeneration=0;
    TArray<FString> GroundChecks;
    int64 Samples=0,HighAltitudeSamples=0;
    int32 PeakVolumes=0,PlumeLights=0,SiteLights=0,PlayingAudio=0;
    int32 StarshipFrames=0,StarshipPlumes=0,SiteDetailInstances=0;
    double MaxStarshipErrorCm=0;
    int32 ChaseContactFrames=0;
    FVector LastChaseOffset=FVector::ZeroVector;
    FQuat LastChaseRotation=FQuat::Identity;
    double MaxChaseContactStepCm=0,MaxChaseContactAngleDeg=0;
    double PeakAltitude=0,PeakSpeed=0,MaxCameraErrorCm=0,MaxAngleErrorDeg=0;
    bool bEnabled=false,bCaptured=false,bVaporLit=false,bVaporHasDensity=false;
};

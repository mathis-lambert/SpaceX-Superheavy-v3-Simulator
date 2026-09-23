#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Recovery/Presentation/RecoveryCameraTracking.h"
#include "RecoveryCameraComponent.generated.h"

class ACameraActor;

/** Viewer state and framing, evaluated after the physical vehicle presentation. */
UCLASS(ClassGroup = (Recovery))
class SUPERHEAVYSIM_API URecoveryCameraComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryCameraComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* TickFunction) override;
    void UpdateCamera(double DeltaSeconds);
    void CycleCamera();
    void ToggleFreeCamera();
    FString GetCameraLabel() const;
    static constexpr int32 CameraCount = 14;
    static TArray<FString> GetCameraNames();
    int32 GetCameraMode() const { return CameraMode; }
    FVector GetViewerFocus() const { return LastCameraFocus; }
    void SetCameraMode(int32 Mode)
    {
        CameraMode = FMath::Clamp(Mode, 0, CameraCount - 1);
        CameraZoom = 1;
    }
    bool bFrontendView = false;

private:
    UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
    int32 CameraMode = 0, PreviousCameraMode = 0, LastCameraMode = -1;
    double CameraTransitionRemaining = 0;
    bool bCameraInitialized = false;
    double CameraZoom = 1, FreeCameraSpeedMps = 50, StageFraming = 0;
    double OrbitYaw = 0, OrbitPitch = 0, CinematicAzimuth = -0.85;
    FRecoveryOrbitInput OrbitInput;
    double SmoothedCameraZoom = 1;
    bool bOrbitManuallyAdjusted = false;
    FRecoveryChaseTracking ChaseTracking;
    FVector CameraBlendOffset = FVector::ZeroVector, CameraLookBlend = FVector::ZeroVector,
            LastCameraFocus = FVector::ZeroVector;
    bool bChaseReview = false, bEarthReview = false, bIgnoreCameraInput = false;
    uint32 LastMissionGeneration = MAX_uint32;
};

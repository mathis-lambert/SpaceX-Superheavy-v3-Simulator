#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Recovery/Interface/RecoveryInput.h"
#include "RecoveryPlayerController.generated.h"

class ASuperHeavyRecoveryDirector;
class SRecoveryMenu;

/** Owns frontend, genuine world pause and persistent local display preferences. */
UCLASS()
class SUPERHEAVYSIM_API ARecoveryPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ARecoveryPlayerController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    static bool ShouldShowFrontend();
    bool IsMenuOpen() const { return bMenuOpen; }
    bool IsAtHome() const { return bAtHome; }
    ASuperHeavyRecoveryDirector* GetDirector() const;
    void TogglePauseMenu();
    void ToggleCameraPicker();
    void ChooseCamera(int32 Mode);
    void LaunchFlight();
    void ResumeFlight();
    void ReturnHome();
    void RestartFlight();
    void QuitSimulation();
    void SavePreferences();
    void SetReconstruction(int32 Mode);
    void SetHardwareRayTracing(bool Enabled);
    void ToggleFlightLab();
    void HandleViewerAction(RecoveryInput::EAction Action);
    void ToggleForceOverlay() { bForceOverlay=!bForceOverlay; }
    bool bHardwareRayTracing=false;
    bool bForceOverlay=false;
    float ForceVectorScale=1.f;
    int32 ReconstructionMode=0;
    void SetTelemetry(bool bEnabled);
    void SetPlaybackRate(float Rate);
    void SlowerPlayback() { SetPlaybackRate(PlaybackRate*0.5f); }
    void FasterPlayback() { SetPlaybackRate(PlaybackRate*2.f); }
    int32 SelectedScenario=0;
    int32 StartingCamera=0;
    bool bTelemetry=true;
    float EngineLightScale=1.f;
    float TimeOfDay=17.9f;
    float FogAmount=1.f;
    float MotionBlur=0.25f;
    float CameraGrain=0.12f;
    bool bCameraDepthOfField=true;
    float MouseSensitivity=0.65f;
    float PlaybackRate=1.f;
    float EffectivePlaybackRate=1.f;
    float MasterVolume=0.75f;
    bool bAutomaticOrbit=true;
    bool bVideoConfirmation=false;
    double VideoConfirmDeadline=0;
    void BeginVideoConfirmation(FIntPoint PreviousResolution,int32 PreviousWindowMode);
    void ConfirmVideo();
    void RevertVideo();
private:
    void SetMenuVisible(bool bVisible);
    void TickInterfaceAudit();
    void TickEnvironmentAudit();
    void TickOverhaulAudit();
    void TickWorldAudit();
    void TickControlsAudit();
    int32 AuditStage=0;
    int32 AuditPendingCamera=-1;
    double AuditDeadline=0,AuditMissionTime=0;
    FVector AuditPosition=FVector::ZeroVector;
    FRotator AuditRotation=FRotator::ZeroRotator;
    FIntPoint AuditResolution;
    int32 AuditWindowMode=2;
    bool bAuditPassed=true;
    TArray<FString> AuditChecks;
    FIntPoint PreviousVideoResolution=FIntPoint(1920,1080);
    int32 PreviousVideoWindowMode=2;
    TSharedPtr<SRecoveryMenu> Menu;
    bool bReconstructionInitialized=false;
    bool bMenuOpen=false,bAtHome=true,bFrontendInitialized=false;
};

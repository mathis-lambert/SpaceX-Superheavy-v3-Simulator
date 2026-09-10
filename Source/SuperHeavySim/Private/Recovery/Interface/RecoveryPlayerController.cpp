#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Presentation/RecoveryStartupSubsystem.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Interface/RecoveryFlightDeck.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Presentation/RecoveryRenderSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"

ARecoveryPlayerController::ARecoveryPlayerController()
{ PrimaryActorTick.bTickEvenWhenPaused=true; bShouldPerformFullTickWhenPaused=true;bEnableMotionControls=false; }

bool ARecoveryPlayerController::ShouldShowFrontend()
{
    return FApp::CanEverRender() && !IsRunningCommandlet() &&
        !FParse::Param(FCommandLine::Get(),TEXT("RecoveryAutoExit")) &&
        !FParse::Param(FCommandLine::Get(),TEXT("RecoveryReview")) &&
        !FParse::Param(FCommandLine::Get(),TEXT("RecoveryNoMenu"));
}
ASuperHeavyRecoveryDirector* ARecoveryPlayerController::GetDirector() const
{
    for(TActorIterator<ASuperHeavyRecoveryDirector> It(GetWorld());It;++It) return *It;
    return nullptr;
}
void ARecoveryPlayerController::BeginPlay()
{
    Super::BeginPlay();
    // The renderer must keep updating view history when optics are edited in
    // pause; otherwise the final zoom velocity remains frozen in motion blur.
    GetWorld()->bIsCameraMoveableWhenPaused=true;
    Photography.Load(TEXT("Recovery.Photography"),GGameUserSettingsIni);
    GConfig->GetInt(TEXT("Recovery.Rendering"),TEXT("Reconstruction"),ReconstructionMode,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Recovery.Rendering"),TEXT("HardwareRayTracing"),bHardwareRayTracing,GGameUserSettingsIni);
    int32 RayTracingOverride=-1;FParse::Value(FCommandLine::Get(),TEXT("RecoveryRayTracing="),RayTracingOverride);
    if(RayTracingOverride>=0)bHardwareRayTracing=RayTracingOverride>0;
    FParse::Value(FCommandLine::Get(),TEXT("RecoveryReconstruction="),ReconstructionMode);
    GConfig->GetFloat(TEXT("Recovery.Presentation"),TEXT("TimeOfDay"),TimeOfDay,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("Recovery.Presentation"),TEXT("FogAmount"),FogAmount,GGameUserSettingsIni);
    GConfig->GetInt(TEXT("Recovery.Presentation"),TEXT("WeatherPreset"),WeatherPreset,GGameUserSettingsIni);WeatherPreset=FMath::Clamp(WeatherPreset,0,3);
    GConfig->GetFloat(TEXT("Recovery.Presentation"),TEXT("MotionBlur"),MotionBlur,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("Recovery.Presentation"),TEXT("CameraGrain"),CameraGrain,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Recovery.Presentation"),TEXT("DepthOfField"),bCameraDepthOfField,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("Recovery.Controls"),TEXT("MouseSensitivity"),MouseSensitivity,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Recovery.Controls"),TEXT("AutomaticOrbitV2"),bAutomaticOrbit,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("Recovery.Audio"),TEXT("MasterVolume"),MasterVolume,GGameUserSettingsIni);
    MouseSensitivity=FMath::Clamp(MouseSensitivity,0.1f,2.f);
    MasterVolume=FMath::Clamp(MasterVolume,0.f,1.f);
    FParse::Value(FCommandLine::Get(),TEXT("RecoveryHour="),TimeOfDay);
    TimeOfDay=FMath::Clamp(TimeOfDay,0.f,24.f);FogAmount=FMath::Clamp(FogAmount,0.f,2.f);
    MotionBlur=FMath::Clamp(MotionBlur,0.f,0.5f);CameraGrain=FMath::Clamp(CameraGrain,0.f,0.35f);
    // Rendering settings also apply to direct launches, replays and audits.
    // Resolution was applied during startup; preserve explicit launch flags.
    if(auto* Settings=GEngine->GetGameUserSettings()) Settings->ApplyNonResolutionSettings();
    ReconstructionMode=RecoveryRenderSettings::ApplyReconstruction(ReconstructionMode);
    bReconstructionInitialized=RecoveryRenderSettings::IsReconstructionReady();
    bHardwareRayTracing=RecoveryRenderSettings::ApplyHardwareRayTracing(bHardwareRayTracing);
    if(!ShouldShowFrontend()) { bAtHome=false;return; }
    GConfig->GetInt(TEXT("Recovery.Interface"),TEXT("Scenario"),SelectedScenario,GGameUserSettingsIni);
    GConfig->GetInt(TEXT("Recovery.Interface"),TEXT("Camera"),StartingCamera,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Recovery.Interface"),TEXT("Telemetry"),bTelemetry,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("Recovery.Interface"),TEXT("EngineLightScale"),EngineLightScale,GGameUserSettingsIni);
    SelectedScenario=FMath::Clamp(SelectedScenario,0,2);StartingCamera=FMath::Clamp(StartingCamera,0,ASuperHeavyRecoveryDirector::CameraCount-1);
    EngineLightScale=FMath::Clamp(EngineLightScale,0.f,2.f);
}
void ARecoveryPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&ARecoveryPlayerController::BeginOrbitDrag);
    InputComponent->BindKey(EKeys::RightMouseButton,IE_Released,this,&ARecoveryPlayerController::EndOrbitDrag);
    InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&ARecoveryPlayerController::SelectUnderCursor);
    for(const auto& Definition:RecoveryInput::Bindings())
    {
        FInputKeyBinding Binding(FInputChord(Definition.Key),IE_Pressed);
        Binding.bConsumeInput=true;
        Binding.bExecuteWhenPaused=Definition.Action==RecoveryInput::EAction::Pause;
        Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this,Action=Definition.Action](){HandleViewerAction(Action);});
        InputComponent->KeyBindings.Add(MoveTemp(Binding));
    }
}
void ARecoveryPlayerController::HandleViewerAction(RecoveryInput::EAction Action)
{
    if(!URecoveryStartupSubsystem::IsReady(GetWorld()))return;
    using namespace RecoveryInput;
    if(Action==EAction::Pause){TogglePauseMenu();return;}
    if(bMenuOpen || bAtHome || IsPaused())return;
    auto* D=GetDirector();if(!D)return;
    switch(Action)
    {
    case EAction::Cameras:ToggleCameraPicker();break;
    case EAction::NextCamera:D->CycleCamera();break;
    case EAction::FreeCamera:D->ToggleFreeCamera();break;
    case EAction::Telemetry:SetTelemetry(!D->bShowTelemetry);break;
    case EAction::Inspect:ToggleForceOverlay();break;
    case EAction::Computer:ToggleFlightComputer();break;
    case EAction::Slower:SlowerPlayback();break;
    case EAction::Faster:FasterPlayback();break;
    case EAction::Start:D->StartMission();break;
    default:break;
    }
}
void ARecoveryPlayerController::PlayerTick(float Dt)
{
    Super::PlayerTick(Dt);
    if(bOrbitDragging && (!IsInputKeyDown(EKeys::RightMouseButton) || !FSlateApplication::Get().IsActive()))EndOrbitDrag();
    if(IsPaused())if(auto* D=GetDirector())D->RefreshViewerCamera(FApp::GetDeltaTime());
    if(!bReconstructionInitialized && RecoveryRenderSettings::IsReconstructionReady())
    {
        ReconstructionMode=RecoveryRenderSettings::ApplyReconstruction(ReconstructionMode);
        bReconstructionInitialized=true;
    }
    if(!URecoveryStartupSubsystem::IsReady(GetWorld()))return;
    if(!IsPaused() && Dt>0)
    {
        // Increase simulated time without exceeding the 15 Hz control interval
        // covered by the flight matrix. Chaos retains its 120 Hz substeps.
        const double WallStep=Dt/FMath::Max(.01f,UGameplayStatics::GetGlobalTimeDilation(this));
        const float Maximum=FMath::Clamp(float((1./15.)/FMath::Max(.001,WallStep)),.25f,4.f);
        EffectivePlaybackRate=FMath::Min(PlaybackRate,Maximum);
        UGameplayStatics::SetGlobalTimeDilation(this,EffectivePlaybackRate);
    }
    // Wait for the director's BeginPlay, including removal of legacy widgets.
    if(!bFrontendInitialized && ShouldShowFrontend() && GetDirector() && GetDirector()->GetBody() && GEngine->GameViewport)
    {
        bFrontendInitialized=true;
        Menu=SNew(SRecoveryMenu).Controller(this);
        GEngine->GameViewport->AddViewportWidgetContent(Menu.ToSharedRef(),100);
        FlightDeck=SNew(SRecoveryFlightDeck).Controller(this);
        GEngine->GameViewport->AddViewportWidgetContent(FlightDeck.ToSharedRef(),30);
        ReturnHome();
    }
    // Real time continues while the flight is paused, unlike world timers.
    if(bVideoConfirmation && FPlatformTime::Seconds()>=VideoConfirmDeadline) RevertVideo();
    if(bFrontendInitialized && FParse::Param(FCommandLine::Get(),TEXT("RecoveryUIAudit"))) TickInterfaceAudit();
    if(bFrontendInitialized && FParse::Param(FCommandLine::Get(),TEXT("RecoveryEarthAudit"))) TickEnvironmentAudit();
    if(bFrontendInitialized && FParse::Param(FCommandLine::Get(),TEXT("RecoveryOverhaulAudit"))) TickOverhaulAudit();
    if(bFrontendInitialized && FParse::Param(FCommandLine::Get(),TEXT("RecoveryWorldAudit"))) TickWorldAudit();
    if(bFrontendInitialized && FParse::Param(FCommandLine::Get(),TEXT("RecoveryPhotoAudit"))) TickPhotographyAudit();
    if(bFrontendInitialized && FParse::Param(FCommandLine::Get(),TEXT("RecoveryInteractiveAudit"))) TickInteractiveAudit();
    if(bFrontendInitialized && (FParse::Param(FCommandLine::Get(),TEXT("RecoveryControlsAudit")) ||
        FParse::Param(FCommandLine::Get(),TEXT("RecoveryVaporReview")))) TickControlsAudit();
}
void ARecoveryPlayerController::SetMenuVisible(bool bVisible)
{
    EndOrbitDrag();bMenuOpen=bVisible;bShowMouseCursor=true;
    if(Menu) Menu->SetVisibility(bVisible?EVisibility::Visible:EVisibility::Collapsed);
    if(auto* D=GetDirector())
    {
        if(bVisible && !bAtHome) bTelemetry=D->bShowTelemetry;
        D->bFrontendView=bAtHome;
    }
    if(bVisible && Menu)
    {
        FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Menu);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Mode);
        FSlateApplication::Get().SetKeyboardFocus(Menu);
    }
    else
    {
        FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
        if(GEngine && GEngine->GameViewport)
        {
            GEngine->GameViewport->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringRightMouseDown);
            GEngine->GameViewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("RECOVERY_UI menu=%d home=%d paused=%d"),bMenuOpen,bAtHome,IsPaused());
}
void ARecoveryPlayerController::TogglePauseMenu()
{
    if(!bMenuOpen && (Selection.IsValid() || bFlightComputer)) {Selection={};bFlightComputer=false;if(FlightDeck)FlightDeck->Refresh();return;}
    if(bVideoConfirmation) { RevertVideo();return; }
    if(!Menu) return;
    if(bAtHome) { Menu->ShowPage();return; }
    if(bMenuOpen) ResumeFlight();
    else { SetPause(true);Menu->ShowPage();SetMenuVisible(true); }
}
void ARecoveryPlayerController::LaunchFlight()
{
    if(!URecoveryStartupSubsystem::IsReady(GetWorld()))return;
    if(auto* D=GetDirector())
    {
        SetPause(false);
        const FString ExpectedScenario=SelectedScenario==1?TEXT("Crosswind"):SelectedScenario==2?TEXT("Offset"):TEXT("Nominal");
        if(D->Phase!=ERecoveryPhase::Ready || D->ScenarioName!=ExpectedScenario)D->SelectScenario(SelectedScenario);
        Selection={};bFlightComputer=false;
        SetWeatherPreset(WeatherPreset);
        D->SetCameraMode(StartingCamera);
        D->bShowTelemetry=bTelemetry;bAtHome=false;SetMenuVisible(false);D->StartMission();SavePreferences();
    }
}
void ARecoveryPlayerController::ToggleCameraPicker()
{
    if(bAtHome || !Menu || bVideoConfirmation) return;
    if(bMenuOpen && !IsPaused()) { ResumeFlight();return; }
    Menu->ShowPage(6);SetMenuVisible(true);
}
void ARecoveryPlayerController::ChooseCamera(int32 Mode)
{
    if(auto* D=GetDirector()) { if(Mode==8 && D->GetCameraMode()!=8) D->ToggleFreeCamera();else D->SetCameraMode(Mode); }
    ResumeFlight();
}
void ARecoveryPlayerController::ResumeFlight()
{ if(!bAtHome) { SetPause(false);SetMenuVisible(false); } }
void ARecoveryPlayerController::ReturnHome()
{
    if(bVideoConfirmation) RevertVideo();
    SetPause(false);bAtHome=true;Selection={};bFlightComputer=false;
    if(auto* D=GetDirector()) { D->SelectScenario(SelectedScenario);D->bShowTelemetry=bTelemetry; }
    if(Menu) Menu->ShowPage();
    SetMenuVisible(true);
}
void ARecoveryPlayerController::RestartFlight() { LaunchFlight(); }
void ARecoveryPlayerController::SetPlaybackRate(float Rate)
{
    if(!FMath::IsFinite(Rate))return;
    PlaybackRate=FMath::Clamp(Rate,0.25f,4.f);
    UE_LOG(LogTemp,Display,TEXT("RECOVERY_PLAYBACK requested=%.2f"),PlaybackRate);
    EffectivePlaybackRate=FMath::Min(PlaybackRate,EffectivePlaybackRate);
    UGameplayStatics::SetGlobalTimeDilation(this,EffectivePlaybackRate);
}
void ARecoveryPlayerController::ToggleFlightComputer()
{
    if(bAtHome || bVideoConfirmation)return;
    bFlightComputer=!bFlightComputer;Selection={};if(FlightDeck)FlightDeck->Refresh();
}
void ARecoveryPlayerController::QuitSimulation()
{ UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false); }
void ARecoveryPlayerController::SetTelemetry(bool bEnabled)
{ bTelemetry=bEnabled;if(auto* D=GetDirector()) D->bShowTelemetry=bEnabled;SavePreferences(); }
void ARecoveryPlayerController::SavePreferences()
{
    // Render/input audits may vary scene settings, but never persist test values.
    if(FString(FCommandLine::Get()).Contains(TEXT("Audit")) || FParse::Param(FCommandLine::Get(),TEXT("RecoveryVaporReview")))return;
    GConfig->SetInt(TEXT("Recovery.Rendering"),TEXT("Reconstruction"),ReconstructionMode,GGameUserSettingsIni);
    Photography.Sanitize();Photography.Save(TEXT("Recovery.Photography"),GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Recovery.Rendering"),TEXT("HardwareRayTracing"),bHardwareRayTracing,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Recovery.Interface"),TEXT("Scenario"),SelectedScenario,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Recovery.Interface"),TEXT("Camera"),StartingCamera,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Recovery.Interface"),TEXT("Telemetry"),bTelemetry,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Interface"),TEXT("EngineLightScale"),EngineLightScale,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Presentation"),TEXT("TimeOfDay"),TimeOfDay,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Presentation"),TEXT("FogAmount"),FogAmount,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("Recovery.Presentation"),TEXT("WeatherPreset"),WeatherPreset,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Presentation"),TEXT("MotionBlur"),MotionBlur,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Presentation"),TEXT("CameraGrain"),CameraGrain,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Recovery.Presentation"),TEXT("DepthOfField"),bCameraDepthOfField,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Controls"),TEXT("MouseSensitivity"),MouseSensitivity,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Recovery.Controls"),TEXT("AutomaticOrbitV2"),bAutomaticOrbit,GGameUserSettingsIni);
    GConfig->RemoveKey(TEXT("Recovery.Controls"),TEXT("LearningOverlay"),GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("Recovery.Audio"),TEXT("MasterVolume"),MasterVolume,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void ARecoveryPlayerController::SetReconstruction(int32 Mode)
{
    ReconstructionMode=RecoveryRenderSettings::ApplyReconstruction(Mode);
    bReconstructionInitialized=RecoveryRenderSettings::IsReconstructionReady();
    SavePreferences();
}
void ARecoveryPlayerController::SetHardwareRayTracing(bool Enabled)
{
    bHardwareRayTracing=RecoveryRenderSettings::ApplyHardwareRayTracing(Enabled);SavePreferences();
}
void ARecoveryPlayerController::BeginVideoConfirmation(FIntPoint PreviousResolution,int32 PreviousWindowMode)
{
    PreviousVideoResolution=PreviousResolution;PreviousVideoWindowMode=PreviousWindowMode;
    bVideoConfirmation=true;VideoConfirmDeadline=FPlatformTime::Seconds()+15;if(Menu) Menu->ShowPage(4);
}
void ARecoveryPlayerController::ConfirmVideo()
{
    if(auto* S=GEngine->GetGameUserSettings()) { S->ConfirmVideoMode();S->SaveSettings(); }
    bVideoConfirmation=false;if(Menu) Menu->ShowPage(2);
}
void ARecoveryPlayerController::RevertVideo()
{
    // GameEngine's resize callback confirms a window resize automatically. Keep
    // our own snapshot so that callback cannot overwrite the rollback target.
    if(auto* S=GEngine->GetGameUserSettings())
    {
        S->SetScreenResolution(PreviousVideoResolution);S->SetFullscreenMode(EWindowMode::Type(PreviousVideoWindowMode));
        S->ApplyResolutionSettings(false);S->ConfirmVideoMode();S->SaveSettings();
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_UI display restored=%dx%d mode=%d"),PreviousVideoResolution.X,PreviousVideoResolution.Y,PreviousVideoWindowMode);
    }
    bVideoConfirmation=false;if(Menu) Menu->ShowPage(2);
}
void ARecoveryPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    EndOrbitDrag();
    if(FlightDeck && GEngine && GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(FlightDeck.ToSharedRef());
    FlightDeck.Reset();
    if(bVideoConfirmation) RevertVideo();
    if(Menu && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
    Menu.Reset();Super::EndPlay(Reason);
}

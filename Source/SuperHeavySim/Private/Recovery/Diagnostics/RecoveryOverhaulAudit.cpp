#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "GameFramework/WorldSettings.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/LightComponent.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"

void ARecoveryPlayerController::TickOverhaulAudit()
{
    const double Now=FPlatformTime::Seconds();auto* D=GetDirector();if(!D || !Menu)return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/Overhaul");
    const auto Shot=[&](const TCHAR* Name){IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/FString(Name)+TEXT(".png"),true,false);};
    const auto Check=[&](bool Passed,const TCHAR* Label){bAuditPassed&=Passed;AuditChecks.Add(FString(Passed?TEXT("PASS "):TEXT("FAIL "))+Label);};
    const auto Noon=[&](){bool Found=false;for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)if(It->GetLightComponent()->Intensity>1000){Found=It->GetActorRotation().Pitch<-55;break;}return Found;};
    if(AuditStage==0){AuditStage=1;AuditDeadline=Now+30;TimeOfDay=17.9f;return;}
    if(Now<AuditDeadline)return;
    // Screenshots consume the following rendered frame: change scene state
    // only in a separate stage, after that image has been captured.
    AuditDeadline=Now+1;
    switch(AuditStage++)
    {
    case 1:Shot(TEXT("HomeSunset"));break;
    case 2:Menu->ShowPage(ERecoveryMenuPage::Photography);AuditDeadline=Now+2;break;
    case 3:Shot(TEXT("LightControls"));break;
    case 4:TimeOfDay=12;AuditDeadline=Now+5;break;
    case 5:Check(Noon(),TEXT("Noon sunlight follows solar elevation"));Shot(TEXT("Noon"));break;
    case 6:TimeOfDay=18.15f;AuditDeadline=Now+5;break;
    case 7:Shot(TEXT("Dusk"));break;
    case 8:TimeOfDay=6.5f;AuditDeadline=Now+5;break;
    case 9:Shot(TEXT("Dawn"));break;
    case 10:TimeOfDay=21;AuditDeadline=Now+5;break;
    case 11:Shot(TEXT("Night"));break;
    case 12:TimeOfDay=17.9f;LaunchFlight();AuditDeadline=Now+FRecoveryLaunchSequence::DurationS+12;break;
    case 13:
    {
        const auto* Vapor=D->FindComponentByClass<URecoveryVaporComponent>();
        Check(Vapor && Vapor->HasRenderableDensity(),TEXT("Visible vapor uses a volume material with nonzero extinction"));
        Shot(TEXT("Liftoff"));break;
    }
    case 14:TogglePauseMenu();Menu->ShowPage(ERecoveryMenuPage::Photography);break;
    case 15:AuditMissionTime=D->MissionTime;TimeOfDay=12;AuditDeadline=Now+3;break;
    case 16:
        Check(IsPaused() && FMath::Abs(D->MissionTime-AuditMissionTime)<0.001,TEXT("Lighting controls preserve paused flight time"));
        Check(Noon(),TEXT("Solar light updates while flight is paused"));Shot(TEXT("PausedLight"));break;
    case 17:TimeOfDay=17.9f;ResumeFlight();AuditDeadline=Now+3;break;
    case 18:
        Check(D->MissionTime>AuditMissionTime,TEXT("Flight resumes after lighting changes"));
        Check(IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricRenderTarget.Mode"))->GetInt()==0,TEXT("Reactive cloud tracing enabled"));
        for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It) if(It->bUnbound) {Check(FMath::Abs(It->Settings.FilmGrainIntensity-CameraGrain)<0.001,TEXT("Camera grain applied to scene postprocess"));break;}
        ReturnHome();Menu->ShowPage();AuditDeadline=Now+2;break;
    case 19:Menu->ShowPage(ERecoveryMenuPage::Settings);break;
    case 20:Shot(TEXT("SettingsHierarchy"));break;
    case 21:Menu->ShowPage(ERecoveryMenuPage::Controls);break;
    case 22:Shot(TEXT("CameraControls"));break;
    case 23:Menu->ShowPage(ERecoveryMenuPage::Audio);break;
    case 24:Shot(TEXT("AudioControls"));break;
    case 25:Menu->ShowPage(ERecoveryMenuPage::About);break;
    case 26:Shot(TEXT("MissionBriefing"));break;
    case 27:TimeOfDay=21;LaunchFlight();AuditDeadline=Now+FRecoveryLaunchSequence::DurationS+11;break;
    case 28:Shot(TEXT("NightLiftoff"));SetPlaybackRate(.25f);break;
    case 29:
        Check(FMath::Abs(GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation()-.25f)<.001f,TEXT("Interactive playback rate changes world simulation speed"));
        SetPlaybackRate(1);TogglePauseMenu();Menu->ShowPage(ERecoveryMenuPage::Mission);break;
    case 30:Shot(TEXT("InteractiveFlight"));break;
    case 31:bAutomaticOrbit=false;ChooseCamera(11);AuditDeadline=Now+3;break;
    case 32:
        AuditRotation=GetViewTarget()->GetActorRotation();
        InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX,IE_Axis,100,1));break;
    case 33:
        Check(!IsInputKeyDown(EKeys::RightMouseButton) && FMath::Abs(FMath::FindDeltaAngleDegrees(AuditRotation.Yaw,GetViewTarget()->GetActorRotation().Yaw))<.1,TEXT("Pointing without RMB leaves orbit camera stationary"));
        ChooseCamera(8);AuditDeadline=Now+2;break;
    case 34:
        AuditRotation=GetViewTarget()->GetActorRotation();
        InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX,IE_Axis,100,1));break;
    case 35:
        Check(!IsInputKeyDown(EKeys::RightMouseButton) && FMath::Abs(FMath::FindDeltaAngleDegrees(AuditRotation.Yaw,GetViewTarget()->GetActorRotation().Yaw))<.1,TEXT("Pointing without RMB leaves free camera stationary"));
        break;
    case 36:InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Pressed,1,1));break;
    case 37:
        AuditRotation=GetViewTarget()->GetActorRotation();
        InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX,IE_Axis,100,1));break;
    case 38:
        Check(FMath::Abs(FMath::FindDeltaAngleDegrees(AuditRotation.Yaw,GetViewTarget()->GetActorRotation().Yaw))>1,TEXT("Holding RMB rotates the camera"));
        InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton,IE_Released,0,1));break;
    case 39:
        Check(bShowMouseCursor && !IsOrbitDragging(),TEXT("Releasing RMB restores the pointer"));ReturnHome();break;
    default:
        TSharedRef<FJsonObject> R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),bAuditPassed);
        TArray<TSharedPtr<FJsonValue>> Values;for(const auto& C:AuditChecks)Values.Add(MakeShared<FJsonValueString>(C));R->SetArrayField(TEXT("checks"),Values);
        FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));
        FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);break;
    }
}

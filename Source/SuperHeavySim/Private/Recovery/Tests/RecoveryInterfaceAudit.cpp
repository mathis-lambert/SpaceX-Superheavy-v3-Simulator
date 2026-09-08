#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Presentation/RecoveryRenderSettings.h"
#include "HAL/IConsoleManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

// Opt-in rendered integration audit. Exercises the same controller actions as
// the Slate buttons and measures actual Chaos state across a paused interval.
void ARecoveryPlayerController::TickInterfaceAudit()
{
    auto* D=GetDirector();auto* S=GEngine->GetGameUserSettings();if(!D || !S) return;
    const double Now=FPlatformTime::Seconds();if(Now<AuditDeadline) return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/InterfaceAudit");
    auto Check=[this](const TCHAR* Name,bool Passed)
    {
        bAuditPassed&=Passed;AuditChecks.Add(FString::Printf(TEXT("%s: %s"),Name,Passed?TEXT("PASS"):TEXT("FAIL")));
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_UI_CHECK %s %s"),Name,Passed?TEXT("PASS"):TEXT("FAIL"));
    };
    auto Shot=[&Dir](const TCHAR* Name)
    { IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/Name,true,false); };
    auto LitCount=[D](bool bOnlyLit)
    {
        int Count=0;TInlineComponentArray<UPointLightComponent*> Lights(D);
        for(auto* Light:Lights) if(Light->GetName().StartsWith(TEXT("ExhaustLight_")) && (!bOnlyLit || (Light->IsVisible() && Light->Intensity>0))) ++Count;
        return Count;
    };
    switch(AuditStage)
    {
    case 0: AuditDeadline=Now+4;break;
    case 1:
        Check(TEXT("Home waits for launch"),bAtHome && bMenuOpen && !IsPaused() && D->Phase==ERecoveryPhase::Ready && D->MissionTime==0);
        Check(TEXT("33 registered engine lights, all off at home"),LitCount(false)==33 && LitCount(true)==0);
        Shot(TEXT("Home.png"));AuditDeadline=Now+2;break;
    case 2: Menu->ShowPage(1);Shot(TEXT("SimulationSettings.png"));AuditDeadline=Now+2;break;
    case 3: Menu->ShowPage(2);Shot(TEXT("Graphics.png"));AuditDeadline=Now+2;break;
    case 4: LaunchFlight();D->SetCameraMode(0);AuditDeadline=Now+14;break;
    case 5:
        Check(TEXT("Launch starts flight and hides menu"),!bMenuOpen && !bAtHome && D->Phase==ERecoveryPhase::Ascent && D->MissionTime>5);
        Check(TEXT("33 engine lights illuminate ascent"),LitCount(true)==33);
        Shot(TEXT("Liftoff.png"));AuditDeadline=Now+1;break;
    case 6: TogglePauseMenu();AuditDeadline=Now+1;break;
    case 7:
        Check(TEXT("Pause sets actual world pause"),IsPaused() && bMenuOpen);
        AuditMissionTime=D->MissionTime;AuditPosition=D->GetBody()->GetComponentLocation();
        AuditResolution=S->GetScreenResolution();AuditWindowMode=int32(S->GetFullscreenMode());
        S->ConfirmVideoMode();S->SetScreenResolution(AuditResolution==FIntPoint(1280,720)?FIntPoint(1600,900):FIntPoint(1280,720));
        S->SetFullscreenMode(EWindowMode::Windowed);S->ApplyResolutionSettings(false);
        BeginVideoConfirmation(AuditResolution,AuditWindowMode);AuditDeadline=Now+2;break;
    case 8:
        Check(TEXT("Unconfirmed resolution is applied"),S->GetScreenResolution()!=AuditResolution && bVideoConfirmation);
        Shot(TEXT("DisplayConfirmation.png"));AuditDeadline=Now+15;break;
    case 9:
        Check(TEXT("Display timeout works while paused"),!bVideoConfirmation && S->GetScreenResolution()==AuditResolution && int32(S->GetFullscreenMode())==AuditWindowMode);
        Check(TEXT("Mission clock remains frozen"),D->MissionTime==AuditMissionTime);
        Check(TEXT("Physics position remains frozen"),D->GetBody()->GetComponentLocation().Equals(AuditPosition,0.001));
        Menu->ShowPage();Shot(TEXT("Pause.png"));AuditDeadline=Now+2;break;
    case 10: ResumeFlight();AuditDeadline=Now+3;break;
    case 11:
        Check(TEXT("Resume advances the same flight"),!IsPaused() && !bMenuOpen && D->MissionTime>AuditMissionTime+0.2);
        ReturnHome();AuditDeadline=Now+1;break;
    case 12:
        TimeOfDay=17.9f;SetReconstruction(0);Menu->ShowPage(12);AuditDeadline=Now+5;break;
    case 13:
        Check(TEXT("Native reconstruction is active"),ReconstructionMode==0 && RecoveryRenderSettings::ActiveReconstruction()==TEXT("Unreal TSR"));
        Shot(TEXT("ReconstructionNative.png"));AuditDeadline=Now+2;break;
    case 14:
        SetReconstruction(3);Menu->ShowPage(12);AuditDeadline=Now+5;break;
    case 15:
        Check(TEXT("DLSS availability has an explicit fallback"),ReconstructionMode==(RecoveryRenderSettings::SupportsDLSS()?3:0));
        Check(TEXT("Reconstruction preserves the ready physical state"),D->MissionTime==0 && D->Phase==ERecoveryPhase::Ready && D->GetBody()->IsSimulatingPhysics());
        if(RecoveryRenderSettings::SupportsDLSS())
        {
            Check(TEXT("DLSS executes in the renderer"),RecoveryRenderSettings::ActiveReconstruction().Contains(TEXT("NVIDIA")));
            Check(TEXT("Fog resolution compensates for scene reconstruction"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.GridPixelSize"))->GetInt()<=6);
        }
        Shot(TEXT("ReconstructionDLSS.png"));AuditDeadline=Now+2;break;
    case 16:
        SetReconstruction(0);ReturnHome();AuditDeadline=Now+2;break;
    case 17:
    {
        Check(TEXT("Return home resets mission and lights"),bAtHome && bMenuOpen && !IsPaused() && D->MissionTime==0 && D->Phase==ERecoveryPhase::Ready && LitCount(true)==0);
        Shot(TEXT("Home.png"));auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),bAuditPassed);
        Result->SetNumberField(TEXT("paused_mission_time_s"),AuditMissionTime);Result->SetNumberField(TEXT("paused_interval_s"),19);
        TArray<TSharedPtr<FJsonValue>> Checks;for(const auto& Item:AuditChecks) Checks.Add(MakeShared<FJsonValueString>(Item));
        Result->SetArrayField(TEXT("checks"),Checks);FString Json;FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));AuditDeadline=Now+2;break;
    }
    case 18: FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);break;
    default:return;
    }
    ++AuditStage;
}

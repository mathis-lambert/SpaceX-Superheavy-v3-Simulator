#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Components/PrimitiveComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

// Rendered integration audit: the live camera picker must not pause or reset physics,
// and transitions must actually reach distant as well as close views.
void ARecoveryPlayerController::TickEnvironmentAudit()
{
    auto* D=GetDirector();if(!D || !GetViewTarget()) return;
    if(AuditPendingCamera>=0) { ChooseCamera(AuditPendingCamera);AuditPendingCamera=-1; }
    const double Now=FPlatformTime::Seconds();if(Now<AuditDeadline) return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/EarthAudit");
    auto Check=[this](const TCHAR* Name,bool Passed)
    { bAuditPassed&=Passed;AuditChecks.Add(FString::Printf(TEXT("%s: %s"),Name,Passed?TEXT("PASS"):TEXT("FAIL")));UE_LOG(LogTemp,Display,TEXT("RECOVERY_EARTH_CHECK %s %d"),Name,Passed); };
    auto Shot=[&Dir](const TCHAR* Name)
    { IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/Name,true,false); };
    const FVector Camera=GetViewTarget()->GetActorLocation();
    switch(AuditStage)
    {
    case 0:AuditDeadline=Now+25;break;
    case 1:Shot(TEXT("Home.png"));AuditDeadline=Now+2;break;
    case 2:LaunchFlight();D->Viewer->SetCameraMode(0);AuditDeadline=Now+8;break;
    case 3:AuditMissionTime=D->MissionTime;ToggleCameraPicker();AuditDeadline=Now+2;break;
    case 4:
        Check(TEXT("Camera picker stays live"),IsMenuOpen() && !IsPaused() && D->MissionTime>AuditMissionTime);
        Shot(TEXT("CameraPicker.png"));AuditDeadline=Now+2;break;
    case 5:ChooseCamera(9);AuditDeadline=Now+4;break;
    case 6:
        Check(TEXT("3 km spectator selection"),!IsMenuOpen() && D->Viewer->GetCameraMode()==9 && FMath::Abs(Camera.X+290000)<100);
        Shot(TEXT("Spectator3km.png"));AuditPendingCamera=10;AuditDeadline=Now+4;break;
    case 7:
        Check(TEXT("8 km spectator selection"),D->Viewer->GetCameraMode()==10 && FMath::Abs(Camera.X+775000)<100);
        Shot(TEXT("Spectator8km.png"));AuditPendingCamera=11;AuditDeadline=Now+4;break;
    case 8:Shot(TEXT("StarbasePanorama.png"));AuditPendingCamera=13;AuditDeadline=Now+5;break;
    case 9:
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_EARTH_VIEW mode=%d position=%s"),D->Viewer->GetCameraMode(),*Camera.ToString());
        Check(TEXT("Whole Earth camera"),D->Viewer->GetCameraMode()==13 && Camera.Z>900000000);
        Shot(TEXT("EarthGlobe.png"));AuditPendingCamera=0;AuditDeadline=Now+3;break;
    case 10:
        Check(TEXT("Globe to booster transition completes"),(Camera-D->GetBody()->GetComponentLocation()).Size()<150000);
        ChooseCamera(8);AuditDeadline=Now+2;break;
    case 11:
        Check(TEXT("Free camera selected"),D->Viewer->GetCameraMode()==8 && !IsPaused());
        TogglePauseMenu();AuditDeadline=Now+1;break;
    case 12:
        Check(TEXT("Pause still works after picker"),IsPaused() && IsMenuOpen());
        ReturnHome();AuditDeadline=Now+2;break;
    case 13:
    {
        Check(TEXT("Home resets mission after camera changes"),IsAtHome() && D->MissionTime==0);
        auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),bAuditPassed);
        TArray<TSharedPtr<FJsonValue>> Checks;for(const auto& Item:AuditChecks) Checks.Add(MakeShared<FJsonValueString>(Item));
        Result->SetArrayField(TEXT("checks"),Checks);FString Json;FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));AuditDeadline=Now+2;break;
    }
    case 14:FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);break;
    default:return;
    }
    ++AuditStage;
}

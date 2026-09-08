#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Presentation/RecoveryRenderSettings.h"
#include "Components/PrimitiveComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "HAL/IConsoleManager.h"

void ARecoveryPlayerController::TickWorldAudit()
{
    const double Now=FPlatformTime::Seconds();if(Now<AuditDeadline)return;
    auto* D=GetDirector();if(!D || !Menu)return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/WorldAudit");
    auto Shot=[&](const TCHAR* Name){IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/Name,true,false);};
    auto Check=[&](const TCHAR* Name,bool Passed){bAuditPassed&=Passed;AuditChecks.Add(FString::Printf(TEXT("%s: %s"),Name,Passed?TEXT("PASS"):TEXT("FAIL")));UE_LOG(LogTemp,Display,TEXT("WORLD_CHECK %s %d"),Name,Passed);};
    auto PlaceCamera=[&](FVector P,FVector Focus)
    {
        bAtHome=false;SetMenuVisible(false);D->SetCameraMode(8);
        if(auto* C=Cast<ACameraActor>(GetViewTarget())){C->SetActorLocationAndRotation(P,(Focus-P).Rotation());C->GetCameraComponent()->SetFieldOfView(65);}
    };
    switch(AuditStage)
    {
    case 0: TimeOfDay=17.9f;SetReconstruction(3);AuditDeadline=Now+8;break;
    case 1: Shot(TEXT("Home.png"));AuditDeadline=Now+2;break;
    case 2: Menu->ShowPage(2);Shot(TEXT("Display.png"));AuditDeadline=Now+2;break;
    case 3: PlaceCamera(FVector(0,-2000000,7800000),FVector(0,70000000,-4000000));TimeOfDay=14.f;AuditDeadline=Now+8;break;
    case 4: Shot(TEXT("Horizon78km.png"));AuditDeadline=Now+2;break;
    case 5: PlaceCamera(FVector(0,-100000,7800000),FVector(0,0,0));AuditDeadline=Now+6;break;
    case 6: Shot(TEXT("Coast78km.png"));AuditDeadline=Now+2;break;
    case 7: TimeOfDay=1.f;PlaceCamera(FVector(0,-2000000,7800000),FVector(0,70000000,18000000));AuditDeadline=Now+8;break;
    case 8: Shot(TEXT("Stars78km.png"));AuditDeadline=Now+2;break;
    case 9: TimeOfDay=17.9f;ReturnHome();AuditDeadline=Now+6;break;
    case 10: bAtHome=false;SetMenuVisible(false);D->SetCameraMode(4);AuditDeadline=Now+4;break;
    case 11: Shot(TEXT("Conditioning.png"));AuditDeadline=Now+2;break;
    case 12: LaunchFlight();D->SetCameraMode(0);AuditDeadline=Now+13;break;
    case 13: bForceOverlay=true;Shot(TEXT("ForceVectors.png"));AuditDeadline=Now+2;break;
    case 14:
        AuditPendingCamera=D->GetEngines().IndexOfByPredicate([](const FRecoveryEngineState& E){return E.bCentral;});
        D->SetFailedEngine(AuditPendingCamera);D->SetJammedFin(1);D->SetReactionJetsDisabled(true);D->SetAttitudeResponse(.5);D->SetWindScale(2.);
        ToggleFlightLab();AuditMissionTime=D->MissionTime;AuditDeadline=Now+3;break;
    case 15:
        Check(TEXT("Flight laboratory leaves simulation running"),!IsPaused() && bMenuOpen && D->MissionTime>AuditMissionTime+1);
        Check(TEXT("Failed engine closes its physical thrust valve"),D->GetEngines().IsValidIndex(AuditPendingCamera) && D->GetEngines()[AuditPendingCamera].ThrustN<1.);
        Check(TEXT("Jammed fin holds its measured angle"),FMath::Abs(D->GridFinAnglesDeg.Y-D->GetExperiment().JammedFinAngleDeg)<1.e-6);
        Check(TEXT("Experiment parameters reach flight model"),D->GetExperiment().bReactionJetsDisabled && D->GetExperiment().AttitudeResponse==.5 && D->GetExperiment().WindScale==2.);
        Check(TEXT("Force samples have physical sources"),D->GetAppliedForces().Num()>=40);
        Shot(TEXT("FlightLab.png"));AuditDeadline=Now+2;break;
    case 16: ResumeFlight();D->ResetExperiments();bForceOverlay=false;SetPlaybackRate(4);AuditMissionTime=D->MissionTime;AuditDeadline=Now+3;break;
    case 17:
        Check(TEXT("Time acceleration advances the same physical flight"),D->MissionTime>AuditMissionTime+4 && PlaybackRate==4 && EffectivePlaybackRate>1);
        Check(TEXT("Restoring experiments restores actuator availability"),D->GetExperiment().FailedEngine==INDEX_NONE && D->GetExperiment().JammedFin==INDEX_NONE && !D->GetExperiment().bReactionJetsDisabled);
        SetPlaybackRate(1);ReturnHome();SetHardwareRayTracing(true);Menu->ShowPage(14);AuditDeadline=Now+8;break;
    case 18:
        Check(TEXT("Ray tracing has a supported-device fallback"),bHardwareRayTracing==RecoveryRenderSettings::SupportsHardwareRayTracing());
        Check(TEXT("Lumen hardware tracing matches the menu state"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"))->GetInt()==int(bHardwareRayTracing));
        Shot(TEXT("HardwareRayTracing.png"));AuditDeadline=Now+2;break;
    case 19:
        SetHardwareRayTracing(false);ReturnHome();AuditDeadline=Now+3;break;
    case 20:
    {
        auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),bAuditPassed);
        TArray<TSharedPtr<FJsonValue>> Values;for(const auto& Result:AuditChecks)Values.Add(MakeShared<FJsonValueString>(Result));R->SetArrayField(TEXT("checks"),Values);
        FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));
        FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);break;
    }
    default:return;
    }
    ++AuditStage;
}

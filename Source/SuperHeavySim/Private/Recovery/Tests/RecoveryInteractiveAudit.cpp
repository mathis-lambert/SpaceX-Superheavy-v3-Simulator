#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "ShaderCompiler.h"

void ARecoveryPlayerController::TickInteractiveAudit()
{
    const double Now=FPlatformTime::Seconds();if(Now<AuditDeadline)return;
    auto* D=GetDirector();if(!D)return;
    if(GShaderCompilingManager && GShaderCompilingManager->IsCompiling()){AuditDeadline=Now+2;return;}
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/InteractiveAudit");
    IFileManager::Get().MakeDirectory(*Dir,true);
    // Isolated, stationary render probes. Only the observer moves; flight physics
    // is untouched. Start capture after weather, exposure and PSOs have settled.
    if(FParse::Param(FCommandLine::Get(),TEXT("RecoveryCloudBenchmark")))
    {
        if(AuditStage==0)
        {
            bAtHome=false;SetMenuVisible(false);D->SetCameraMode(8);
            bAutomaticOrbit=false;CameraGrain=0;MotionBlur=0;bCameraDepthOfField=false;
            float Height=80000,Pitch=-45,Hour=12;
            FParse::Value(FCommandLine::Get(),TEXT("CloudHeight="),Height);
            FParse::Value(FCommandLine::Get(),TEXT("CloudPitch="),Pitch);
            FParse::Value(FCommandLine::Get(),TEXT("RecoveryHour="),Hour);TimeOfDay=Hour;
            int32 Weather=2;FParse::Value(FCommandLine::Get(),TEXT("CloudWeather="),Weather);
            SetWeatherPreset(Weather);
            if(auto* Camera=Cast<ACameraActor>(GetViewTarget()))
            {
                Camera->SetActorLocationAndRotation(FVector(Height<500?-50000.:0,0,Height*100.),FRotator(Pitch,0,0));
                Camera->GetCameraComponent()->SetFieldOfView(65);
            }
            AuditStage=1;AuditDeadline=Now+15;return;
        }
        if(AuditStage==1)
        {
            ConsoleCommand(TEXT("csvprofile frames=360"),false);
            AuditStage=2;AuditDeadline=Now+2;return;
        }
        if(AuditStage==2)
        {
            FString Name=TEXT("CloudProbe");FParse::Value(FCommandLine::Get(),TEXT("RecoveryReviewName="),Name);
            FScreenshotRequest::RequestScreenshot(Dir/FPaths::MakeValidFileName(Name)+TEXT(".png"),false,false);
            AuditStage=3;
        }
        return;
    }
    const auto Shot=[&](FString Name){FScreenshotRequest::RequestScreenshot(Dir/Name,true,false);};
    const auto Check=[&](const TCHAR* Name,bool Passed){bAuditPassed&=Passed;AuditChecks.Add(FString::Printf(TEXT("%s: %s"),Name,Passed?TEXT("PASS"):TEXT("FAIL")));};
    const auto Planet=[&](double Altitude)
    {
        D->SetCameraMode(8);
        if(auto* Camera=Cast<ACameraActor>(GetViewTarget()))
        {
            const FVector P(0,0,Altitude*100);
            const double Dip=FMath::Acos(6371000./(6371000.+Altitude));
            const FVector Look(FMath::Cos(Dip),0,-FMath::Sin(Dip));
            Camera->SetActorLocationAndRotation(P,Look.Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);
        }
    };
    switch(AuditStage)
    {
    case 0:
        bAtHome=false;SetMenuVisible(false);D->SetCameraMode(0);TimeOfDay=12;CameraGrain=0;MotionBlur=0;bAutomaticOrbit=false;
        Photography.bAutomaticFraming=true;bCameraDepthOfField=false;AuditDeadline=Now+8;
        if(FParse::Param(FCommandLine::Get(),TEXT("RecoverySteamOnly"))){AuditStage=11;return;}break;
    case 1:
        Check(TEXT("Cursor visible in flight view"),bShowMouseCursor && !IsOrbitDragging());
        AuditMissionTime=D->MissionTime;AuditPosition=D->GetBody()->GetComponentLocation();
        SelectPart({ERecoveryPart::Engine,0});SetSelectedPartFault(true);
        Check(TEXT("Selection command reaches engine fault input"),D->GetExperiment().FailedEngine==0);
        Check(TEXT("Selecting and failing an engine does not restart the vehicle"),D->MissionTime==AuditMissionTime && D->GetBody()->GetComponentLocation().Equals(AuditPosition,.1));
        Shot(TEXT("EngineCard.png"));AuditDeadline=Now+2;break;
    case 2:SetSelectedPartFault(false);Check(TEXT("Explicit restore clears fault"),D->GetExperiment().FailedEngine==INDEX_NONE);SelectPart({});ToggleFlightComputer();Shot(TEXT("Computer.png"));AuditDeadline=Now+2;break;
    case 3:ToggleFlightComputer();Planet(10000);AuditDeadline=Now+5;break;
    case 4:Shot(TEXT("Horizon10km.png"));AuditDeadline=Now+1;break;
    case 5:Planet(14000);AuditDeadline=Now+4;break;
    case 6:Shot(TEXT("Horizon14km.png"));AuditDeadline=Now+1;break;
    case 7:Planet(16000);AuditDeadline=Now+4;break;
    case 8:Shot(TEXT("Horizon16km.png"));AuditDeadline=Now+1;break;
    case 9:Planet(80000);AuditDeadline=Now+5;break;
    case 10:Shot(TEXT("Horizon80km.png"));AuditDeadline=Now+1;break;
    case 11:
        if(FParse::Param(FCommandLine::Get(),TEXT("RecoverySkyOnly"))){FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);++AuditStage;return;}
        SelectPart({});D->SetCameraMode(8);D->ResetExperiments();
        if(auto* Camera=Cast<ACameraActor>(GetViewTarget()))
        {
            const FVector Base=D->GetBody()->GetComponentLocation();
            const FVector P=Base+FVector(24000,-32000,13000),Focus=Base+FVector(0,0,3000);
            Camera->SetActorLocationAndRotation(P,(Focus-P).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);
        }
        D->StartMission();AuditPendingCamera=-1;AuditDeadline=Now+1;break;
    case 12:
        if(D->Phase==ERecoveryPhase::Countdown || D->Phase==ERecoveryPhase::Ready || D->MissionTime<1){AuditDeadline=Now+.5;return;}
        Shot(FString::Printf(TEXT("Steam-%03d.png"),AuditPendingCamera+1));
        ++AuditPendingCamera;AuditDeadline=Now+.05;
        if(AuditPendingCamera<30)return;
        {
            const auto* Vapor=D->FindComponentByClass<URecoveryVaporComponent>();
            Check(TEXT("Turbulent steam has visible volumes during launch"),Vapor && Vapor->GetTurbulentVolumeCount()>0);
        }
        break;
    case 13:
        Planet(80000);SetWeatherPreset(0);AuditDeadline=Now+12;break;
    case 14:Shot(TEXT("Clear80km.png"));AuditDeadline=Now+1;break;
    case 15:TimeOfDay=22;Planet(16000);AuditDeadline=Now+8;break;
    case 16:Shot(TEXT("Night16km.png"));AuditDeadline=Now+1;break;
    case 17:TimeOfDay=12;SetWeatherPreset(1);Planet(3000);AuditDeadline=Now+12;break;
    case 18:Shot(TEXT("CoastalHaze.png"));AuditDeadline=Now+1;break;
    case 19:SetWeatherPreset(3);AuditDeadline=Now+12;break;
    case 20:Shot(TEXT("Overcast.png"));AuditDeadline=Now+1;break;
    case 21:
    {
        auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),bAuditPassed);
        TArray<TSharedPtr<FJsonValue>> Checks;for(const auto& C:AuditChecks)Checks.Add(MakeShared<FJsonValueString>(C));R->SetArrayField(TEXT("checks"),Checks);
        FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));
        FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);break;
    }
    default:return;
    }
    ++AuditStage;
}

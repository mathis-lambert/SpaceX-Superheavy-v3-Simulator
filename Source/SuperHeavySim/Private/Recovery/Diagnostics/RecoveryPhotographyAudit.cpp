#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Presentation/RecoverySolarPosition.h"
#include "Recovery/Presentation/RecoverySiteActivityComponent.h"
#include "Recovery/Presentation/RecoverySiteDetailsComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

// Opt-in end-to-end presentation audit. Run with an isolated -UserDir so the
// persistence checks never overwrite a viewer's saved photographic looks.
void ARecoveryPlayerController::TickPhotographyAudit()
{
    const double Now=FPlatformTime::Seconds();if(Now<AuditDeadline)return;
    auto* D=GetDirector();if(!D || !Menu)return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/Photography");IFileManager::Get().MakeDirectory(*Dir,true);
    const auto Check=[&](const TCHAR* Label,bool Pass){bAuditPassed&=Pass;AuditChecks.Add(FString(Label)+(Pass?TEXT(": PASS"):TEXT(": FAIL")));UE_LOG(LogTemp,Display,TEXT("PHOTO_CHECK %s %d"),Label,Pass);};
    const auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/FString(Name)+TEXT(".png"),true,false);};
    const auto SolarCheck=[&](){
        const FVector Expected=RecoverySolarPosition::Direction(25.9973,-97.1569,TimeOfDay,Photography.UtcOffsetHours,FMath::RoundToInt(Photography.SolarDayOfYear));
        bool Match=false;
        for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)
            if(auto* L=Cast<UDirectionalLightComponent>(It->GetLightComponent());L && L->bAtmosphereSunLight && L->AtmosphereSunLightIndex==0)
                Match=FVector::DotProduct(-It->GetActorForwardVector(),Expected)>.99999;
        Check(TEXT("Rendered sun follows true east/north/up"),Match);
    };
    auto* Camera=Cast<ACameraActor>(GetViewTarget());
    const auto View=[&](FVector Position,FVector Focus){D->Viewer->SetCameraMode(8);if(Camera)Camera->SetActorLocationAndRotation(Position,(Focus-Position).Rotation());};
    const auto Finish=[&](){
        auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),bAuditPassed);
        TArray<TSharedPtr<FJsonValue>> Checks;for(const auto& C:AuditChecks)Checks.Add(MakeShared<FJsonValueString>(C));Report->SetArrayField(TEXT("checks"),Checks);
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));
        FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);
    };
    AuditDeadline=Now+1;
    switch(AuditStage++)
    {
    case 0:
        bAtHome=false;SetMenuVisible(false);SetReconstruction(3);bTelemetry=false;D->bShowTelemetry=false;bAutomaticOrbit=false;
        D->SetWindScale(2);D->Viewer->SetCameraMode(11);TimeOfDay=8;AuditDeadline=Now+12;
        if(FParse::Param(FCommandLine::Get(),TEXT("RecoveryPhotoReload")))AuditStage=40;
        break;
    case 1:SolarCheck();Shot(TEXT("01_Morning"));break;
    case 2:TimeOfDay=13.4f;AuditDeadline=Now+5;break;
    case 3:SolarCheck();Shot(TEXT("02_SolarNoon"));break;
    case 4:TimeOfDay=19.05f;AuditDeadline=Now+5;break;
    case 5:SolarCheck();Shot(TEXT("03_GoldenHour"));break;
    case 6:ApplyPhotoPreset(4);AuditDeadline=Now+7;break;
    case 7:SolarCheck();Shot(TEXT("04_IndustrialNight"));break;
    case 8:ApplyPhotoPreset(0);View(FVector(100000,-155000,62000),FVector(30000,0,0));AuditDeadline=Now+6;break;
    case 9:Shot(TEXT("05_Coast"));break;
    case 10:View(FVector(0,-200000,780000),FVector(0,0,0));AuditDeadline=Now+6;break;
    case 11:Shot(TEXT("06_RegisteredTerrain"));break;
    case 12:ApplyPhotoPreset(1);AuditDeadline=Now+6;break;
    case 13:
        Check(TEXT("Broadcast optics reach the live camera"),Camera && FMath::Abs(Camera->GetCameraComponent()->FieldOfView-Photography.HorizontalFovDegrees())<.05);
        Shot(TEXT("07_Broadcast"));break;
    case 14:ApplyPhotoPreset(2);AuditDeadline=Now+5;break;
    case 15:Shot(TEXT("08_Onboard"));Check(TEXT("Presets preserve the flight experiment"),D->GetExperiment().WindScale==2 && D->Phase==ERecoveryPhase::Ready);break;
    case 16:ApplyPhotoPreset(3);AuditDeadline=Now+5;break;
    case 17:Shot(TEXT("09_GoldenLook"));break;
    case 18:
        TogglePauseMenu();Menu->ShowPage(ERecoveryMenuPage::Optics);Photography.bAutomaticFraming=false;Photography.FocalLengthMm=80;
        Photography.WhiteBalanceK=4800;Photography.ExposureBiasEV=.75f;Photography.Aperture=2.8f;Photography.bAutomaticFocus=false;Photography.FocusDistanceM=350;
        TimeOfDay=12;Photography.MotionStrength=0;bAutomaticOrbit=false;
        AuditPosition=D->GetBody()->GetComponentLocation();AuditMissionTime=D->MissionTime;AuditDeadline=Now+5;break;
    case 19:
    {
        Check(TEXT("Paused camera applies lens changes"),Camera && FMath::Abs(Camera->GetCameraComponent()->FieldOfView-Photography.HorizontalFovDegrees())<.05);
        Check(TEXT("Photographic preview keeps the physical vehicle frozen"),IsPaused() && D->GetBody()->GetComponentLocation().Equals(AuditPosition,.01) && D->MissionTime==AuditMissionTime);
        SolarCheck();bool Match=false;
        for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It)if(It->bUnbound){const auto& P=It->Settings;Match=FMath::IsNearlyEqual(P.WhiteTemp,4800.f) && FMath::IsNearlyEqual(P.DepthOfFieldFstop,2.8f) && FMath::IsNearlyEqual(P.DepthOfFieldFocalDistance,35000.f);}
        Check(TEXT("Live post process receives white balance, aperture and focus"),Match);
        Check(TEXT("Paused preview keeps temporal camera history updating"),GetWorld()->bIsCameraMoveableWhenPaused);
        Menu->ShowPage(ERecoveryMenuPage::Optics);Shot(TEXT("10_OpticsMenu"));break;
    }
    case 20:Menu->ShowPage(ERecoveryMenuPage::Environment);break;
    case 21:Shot(TEXT("11_EnvironmentMenu"));break;
    case 22:
        Photography.FocalLengthMm=145;Photography.SolarDayOfYear=172;Photography.UtcOffsetHours=-6;TimeOfDay=17.25;
        Photography.TrackingLagSeconds=.16f;Photography.bFixedFraming=true;
        bInvertVerticalLook=false;SavePreferences();
        SavePhotoLook(1);ApplyPhotoPreset(0);LoadPhotoLook(1);Menu->ShowPage(ERecoveryMenuPage::SavedLooks);break;
    case 23:
        Check(TEXT("Custom look restores optics, calendar and civil clock"),HasPhotoLook(1) && Photography.FocalLengthMm==145 && Photography.SolarDayOfYear==172 && Photography.UtcOffsetHours==-6 && TimeOfDay==17.25);
        Check(TEXT("Saved look restores tracking character"),Photography.TrackingLagSeconds==.16f && Photography.bFixedFraming);
        Shot(TEXT("12_SavedLooks"));break;
    case 24:
    {
        ResumeFlight();ApplyPhotoPreset(0);D->Viewer->SetCameraMode(11);
        const auto* Activity=D->FindComponentByClass<URecoverySiteActivityComponent>();
        const auto* Detail=D->FindComponentByClass<URecoverySiteDetailsComponent>();
        Check(TEXT("Four service vehicles and two facility vents"),Activity && Activity->GetVehicleCount()==4 && Activity->GetVentCount()==2);
        Check(TEXT("Service vehicles move on the shared road"),Activity && Activity->GetTrafficDistanceM()>5);
        Check(TEXT("Instanced industrial facilities were built"),Detail && Detail->GetInstanceCount()>4000);
        AuditDeadline=Now+3;break;
    }
    case 25:
        TogglePauseMenu();SetMenuVisible(false);Photography.ExposureBiasEV=-1;TimeOfDay=17.9f;CameraGrain=0;AuditDeadline=Now+5;break;
    case 26:Shot(TEXT("13_ExposureLow"));break;
    case 27:Photography.ExposureBiasEV=1;AuditDeadline=Now+5;break;
    case 28:Shot(TEXT("14_ExposureHigh"));break;
    case 29:ResumeFlight();ApplyPhotoPreset(5);AuditDeadline=Now+4;break;
    case 30:
        Check(TEXT("Fixed coastal preset uses a fixed ground camera"),Photography.bFixedFraming && !Photography.bAutomaticFraming && D->Viewer->GetCameraMode()==9);
        Shot(TEXT("15_FixedCoastal"));Finish();break;
    case 40:
        Check(TEXT("Look survives application restart"),HasPhotoLook(1));LoadPhotoLook(1);AuditDeadline=Now+3;break;
    case 41:
        Check(TEXT("Reloaded optics and environment remain exact"),Photography.FocalLengthMm==145 && Photography.SolarDayOfYear==172 && Photography.UtcOffsetHours==-6 && TimeOfDay==17.25);
        Check(TEXT("Tracking character survives application restart"),Photography.TrackingLagSeconds==.16f && Photography.bFixedFraming);
        Check(TEXT("Vertical look inversion survives application restart"),!bInvertVerticalLook);
        Finish();break;
    default:break;
    }
}

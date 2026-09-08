#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

// Exercises real input dispatch, including Slate focus. Direct action calls alone
// cannot detect competing actor bindings (the original F2/F3 regression).
void ARecoveryPlayerController::TickControlsAudit()
{
    const double Now=FPlatformTime::Seconds();if(Now<AuditDeadline)return;
    auto* D=GetDirector();if(!D || !Menu)return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/ControlsAudit");
    IFileManager::Get().MakeDirectory(*Dir,true);
    const auto Check=[&](FString Label,bool Passed){bAuditPassed&=Passed;AuditChecks.Add(Label+(Passed?TEXT(": PASS"):TEXT(": FAIL")));UE_LOG(LogTemp,Display,TEXT("CONTROLS_CHECK %s %d"),*Label,Passed);};
    const auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/Name,true,false);};
    if(FParse::Param(FCommandLine::Get(),TEXT("RecoveryVaporReview")))
    {
        if(AuditStage==0)
        {
            bAtHome=false;SetMenuVisible(false);D->SetCameraMode(8);TimeOfDay=12;SetReconstruction(0);bTelemetry=false;D->bShowTelemetry=false;
            const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody());
            if(auto* C=Cast<ACameraActor>(GetViewTarget())){const FVector P=Base+FVector(3000,-1800,3800);C->SetActorLocationAndRotation(P,(Base+FVector(445,0,4000)-P).Rotation());C->GetCameraComponent()->SetFieldOfView(60);}
            AuditStage=1;AuditDeadline=Now+12;return;
        }
        if(AuditStage==1){Shot(TEXT("VaporDay.png"));AuditStage=2;AuditDeadline=Now+2;return;}
        if(AuditStage==2){TimeOfDay=21;AuditStage=3;AuditDeadline=Now+8;return;}
        if(AuditStage==3){Shot(TEXT("VaporNight.png"));AuditStage=4;AuditDeadline=Now+2;return;}
        if(AuditStage==4)
        {
            TimeOfDay=12;
            const FVector Site=D->Tower->GetActorLocation();
            if(auto* C=Cast<ACameraActor>(GetViewTarget())){const FVector P=Site+FVector(-30000,-14500,1800);C->SetActorLocationAndRotation(P,(Site+FVector(-18000,-10500,350)-P).Rotation());}
            AuditStage=5;AuditDeadline=Now+6;return;
        }
        if(AuditStage==5){Shot(TEXT("SiteActivityDay.png"));AuditStage=6;AuditDeadline=Now+2;return;}
        if(AuditStage==6){TimeOfDay=21;AuditStage=7;AuditDeadline=Now+7;return;}
        if(AuditStage==7){Shot(TEXT("SiteActivityNight.png"));AuditStage=8;AuditDeadline=Now+2;return;}
        FPlatformMisc::RequestExit(false);return;
    }
    static const TArray<FKey> Keys={EKeys::F1,EKeys::F2,EKeys::F3,EKeys::One,EKeys::Two,EKeys::Three,
        EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::R,EKeys::X,
        EKeys::I,EKeys::I,EKeys::L,EKeys::L,EKeys::Tab,EKeys::Tab,EKeys::C,EKeys::F,EKeys::F,
        EKeys::H,EKeys::H,EKeys::J,EKeys::K};
    if(AuditStage==0){TimeOfDay=12;SetReconstruction(3);AuditDeadline=Now+8;AuditStage=1;return;}
    if(AuditStage==1){Shot(TEXT("Home.png"));AuditDeadline=Now+1;AuditStage=2;return;}
    if(AuditStage==2)
    {
        SelectedScenario=1;ReturnHome();bAtHome=false;SetMenuVisible(false);D->SetCameraMode(4);D->SetWindScale(2);
        bForceOverlay=false;bTelemetry=true;D->bShowTelemetry=true;SetPlaybackRate(1);
        AuditMissionTime=D->MissionTime;AuditDeadline=Now+5;AuditStage=3;return;
    }
    if(AuditStage==3){Shot(TEXT("CryogenicClose.png"));AuditStage=4;AuditDeadline=Now+1;return;}
    if(AuditStage==4){Menu->ShowPage(3);SetMenuVisible(true);Shot(TEXT("Controls.png"));AuditStage=5;AuditDeadline=Now+1;return;}
    if(AuditStage==5){ResumeFlight();AuditPendingCamera=0;AuditStage=10;return;}
    if(AuditStage==10 || AuditStage==11)
    {
        const int Index=AuditPendingCamera/2;const bool Press=(AuditPendingCamera%2)==0;
        if(Index>=Keys.Num())
        {
            Check(AuditStage==10?TEXT("Ready keys never reset experiments"):TEXT("Flight keys never reset experiments"),D->GetExperiment().WindScale==2);
            if(AuditStage==10){D->StartMission();AuditMissionTime=D->MissionTime;AuditDeadline=Now+FRecoveryLaunchSequence::DurationS+17;AuditStage=12;return;}
            AuditStage=13;return;
        }
        const FKey Key=Keys[Index];
        if(Press)
        {
            if(bMenuOpen)FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
            else InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1,1));
        }
        else
        {
            InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0,1));
            Check(FString::Printf(TEXT("%s %s preserves mission"),AuditStage==10?TEXT("READY"):TEXT("ASCENT"),*Key.ToString()),
                D->GetExperiment().WindScale==2 && (AuditStage==10?D->Phase==ERecoveryPhase::Ready:D->MissionTime>=AuditMissionTime && D->Phase!=ERecoveryPhase::Ready));
            if(Index==11)
            {
                Check(TEXT("I opens force inspector"),bForceOverlay);
                Shot(AuditStage==10?TEXT("ForcesReady.png"):TEXT("ForcesAscent.png"));
            }
            if(Index==12)Check(TEXT("I closes force inspector"),!bForceOverlay);
            if(Index==13)Check(TEXT("L opens live lab"),bMenuOpen && !IsPaused());
            if(Index==14)Check(TEXT("L closes focused live lab"),!bMenuOpen);
            if(Index==15)Check(TEXT("Tab opens camera picker"),bMenuOpen);
            if(Index==16)Check(TEXT("Tab closes focused camera picker"),!bMenuOpen);
            if(Index==22)Check(TEXT("J halves playback"),PlaybackRate==.5f);
            if(Index==23)Check(TEXT("K restores playback"),PlaybackRate==1.f);
        }
        ++AuditPendingCamera;AuditDeadline=Now+.16;return;
    }
    if(AuditStage==12){Check(TEXT("Launch advanced to ascent"),D->Phase==ERecoveryPhase::Ascent && D->MissionTime>5);AuditMissionTime=D->MissionTime;AuditPendingCamera=0;AuditStage=11;return;}
    if(AuditStage==13){TogglePauseMenu();Shot(TEXT("Pause.png"));AuditDeadline=Now+.5;AuditStage=19;return;}
    if(AuditStage==19){AuditMissionTime=D->MissionTime;AuditPosition=D->GetBody()->GetComponentLocation();AuditDeadline=Now+2;AuditStage=14;return;}
    if(AuditStage==14)
    {
        Check(TEXT("Pause freezes the existing physical flight"),IsPaused() && D->MissionTime==AuditMissionTime && D->GetBody()->GetComponentLocation().Equals(AuditPosition,.01));
        Menu->ShowPage(8);Shot(TEXT("Settings.png"));AuditStage=15;AuditDeadline=Now+1;return;
    }
    if(AuditStage==15){ResumeFlight();ToggleFlightLab();Shot(TEXT("Lab.png"));AuditStage=16;AuditDeadline=Now+1;return;}
    if(AuditStage==16){ResumeFlight();ReturnHome();AuditStage=17;AuditDeadline=Now+7;return;}
    if(AuditStage==17)
    {
        const auto* Vapor=D->FindComponentByClass<URecoveryVaporComponent>();
        Check(TEXT("Two detailed cryogenic volumes are visible"),Vapor && Vapor->GetCryogenicVolumeCount()==2);
        auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),bAuditPassed);
        TArray<TSharedPtr<FJsonValue>> Checks;for(const auto& C:AuditChecks)Checks.Add(MakeShared<FJsonValueString>(C));R->SetArrayField(TEXT("checks"),Checks);
        FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));
        FPlatformMisc::RequestExitWithStatus(false,bAuditPassed?0:1);AuditStage=18;
    }
}

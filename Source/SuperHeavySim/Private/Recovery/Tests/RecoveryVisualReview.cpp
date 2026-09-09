#include "Recovery/Tests/RecoveryDiagnosticsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void URecoveryDiagnosticsComponent::TickVisualReview()
{
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    const auto* PC=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    if(!D || !PC || !PC->PlayerCameraManager || !D->GetBody())return;
    const double Time=D->MissionTime;
    FString Name;
    if(LastReviewPhase!=int32(D->Phase) && D->GetPhaseTimeS()>(D->Phase==ERecoveryPhase::Ascent?8.:1.))
    {LastReviewPhase=int32(D->Phase);Name=D->GetPhaseLabel();}
    if(D->Phase==ERecoveryPhase::Ascent)
    {
        for(double At:{16.,28.,45.,70.,100.})
            if(PreviousReviewTime<At && Time>=At)Name=FString::Printf(TEXT("VFX_Ascent_%03d"),int(At));
        if(FParse::Param(FCommandLine::Get(),TEXT("RecoveryDetailReview")))
            for(double At:{1.,3.,5.})
                if(PreviousReviewTime<At && Time>=At)Name=FString::Printf(TEXT("VFX_Close_%03d"),int(At));
    }
    if(D->GetCameraMode()==12 && PreviousReviewTime<210. && Time>=210.)Name=TEXT("EarthHorizon");
    if(bCloudReview)
    {
        // Matching one-second sequences through the cloud layer, across its top,
        // at the orbital horizon and on descent. Only the camera is scripted;
        // the vehicle runs the same physical mission as the performance capture.
        for(double Start:{27.,35.,42.,210.,242.,317.,321.})
            for(int Sample=0;Sample<10;++Sample)
            {
                const double At=Start+Sample*.1;
                if(PreviousReviewTime<At && Time>=At)
                    Name=FString::Printf(TEXT("Cloud_%03d_%02d"),int(Start),Sample);
            }
    }
    PreviousReviewTime=Time;
    if(Name.IsEmpty())return;
    Name+=TEXT(".png");
    FScreenshotRequest::RequestScreenshot(VisualReviewDirectory/Name,false,false);
    auto Frame=MakeShared<FJsonObject>();
    Frame->SetStringField(TEXT("image"),Name);
    Frame->SetNumberField(TEXT("mission_time_s"),Time);
    Frame->SetNumberField(TEXT("world_time_s"),GetWorld()->GetTimeSeconds());
    Frame->SetNumberField(TEXT("altitude_m"),D->AltitudeM);
    Frame->SetNumberField(TEXT("solar_hour"),PC->TimeOfDay);
    Frame->SetStringField(TEXT("phase"),D->GetPhaseLabel());
    Frame->SetNumberField(TEXT("camera"),D->GetCameraMode());
    Frame->SetNumberField(TEXT("fov_deg"),PC->PlayerCameraManager->GetFOVAngle());
    const auto Vector=[&](const TCHAR* Key,const FVector& V)
    {
        TArray<TSharedPtr<FJsonValue>> Values;
        for(double Value:{V.X,V.Y,V.Z})Values.Add(MakeShared<FJsonValueNumber>(Value));
        Frame->SetArrayField(Key,Values);
    };
    Vector(TEXT("camera_cm"),PC->PlayerCameraManager->GetCameraLocation());
    Vector(TEXT("camera_forward"),PC->PlayerCameraManager->GetCameraRotation().Vector());
    Vector(TEXT("body_cm"),D->GetBody()->GetComponentLocation());
    Vector(TEXT("body_up"),D->GetBody()->GetUpVector());
    if(const auto* Viewport=GetWorld()->GetGameViewport();Viewport && Viewport->Viewport)
    {
        const auto Size=Viewport->Viewport->GetSizeXY();
        Frame->SetNumberField(TEXT("width"),Size.X);Frame->SetNumberField(TEXT("height"),Size.Y);
    }
    for(const TCHAR* Key:{TEXT("r.VolumetricRenderTarget.Mode"),TEXT("r.ScreenPercentage"),TEXT("r.Lumen.HardwareRayTracing")})
        if(const auto* C=IConsoleManager::Get().FindConsoleVariable(Key))Frame->SetNumberField(Key,C->GetFloat());
    ReviewFrames.Add(MakeShared<FJsonValueObject>(Frame));
}

void URecoveryDiagnosticsComponent::WriteVisualReview()
{
    auto Report=MakeShared<FJsonObject>();
    Report->SetNumberField(TEXT("schema_version"),1);
    Report->SetBoolField(TEXT("visual_review_required"),true);
    Report->SetArrayField(TEXT("frames"),ReviewFrames);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(VisualReviewDirectory/TEXT("frames.json")));
}

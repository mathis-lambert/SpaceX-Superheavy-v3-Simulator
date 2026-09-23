#include "Recovery/Diagnostics/RecoveryDiagnosticsComponent.h"
#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Presentation/RecoveryStartupSubsystem.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Presentation/RecoverySiteDetailsComponent.h"
#include "Recovery/Presentation/RecoveryRenderSettings.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/AudioComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"
#include "ShaderPipelineCache.h"
#include "AudioMixerBlueprintLibrary.h"

URecoveryDiagnosticsComponent::URecoveryDiagnosticsComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
void URecoveryDiagnosticsComponent::BeginPlay()
{
    Super::BeginPlay();
    bEnabled=FParse::Param(FCommandLine::Get(),TEXT("RecoveryExperienceAudit"));
    bRecordAudio=FParse::Param(FCommandLine::Get(),TEXT("RecoveryAudioAudit"));
    bGroundAudit=FParse::Param(FCommandLine::Get(),TEXT("RecoveryGroundAudit"));
    bPerformanceAudit=FParse::Param(FCommandLine::Get(),TEXT("RecoveryPerformanceAudit"));
    bEndProfileOnResult=FParse::Param(FCommandLine::Get(),TEXT("RecoveryEndProfileOnResult"));
    bCloudReview=FParse::Param(FCommandLine::Get(),TEXT("RecoveryCloudReview"));
    bVisualReview=bCloudReview || FParse::Param(FCommandLine::Get(),TEXT("RecoveryReview"));
    if(bVisualReview)
    {
        FString Name;FParse::Value(FCommandLine::Get(),TEXT("RecoveryReviewName="),Name);
        VisualReviewDirectory=FPaths::ProjectSavedDir()/TEXT("Recovery/Review")/FPaths::MakeValidFileName(Name);
        IFileManager::Get().MakeDirectory(*VisualReviewDirectory,true);
    }
    SetComponentTickEnabled(bEnabled || bGroundAudit || bPerformanceAudit || bVisualReview);
}
void URecoveryDiagnosticsComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!URecoveryStartupSubsystem::IsReady(GetWorld()))return;
    if(bPerformanceAudit)RecordPerformanceFrame();
    if(bVisualReview)TickVisualReview();
    if(bGroundAudit){TickGroundAudit(Dt);if(!bEnabled)return;}
    if(!bEnabled)return;
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    const auto* PC=GetWorld()->GetFirstPlayerController();
    if(!D || !PC || !PC->PlayerCameraManager || !PC->GetViewTarget() || !D->GetBody())return;
    if(bRecordAudio && !bAudioRecorded)
    {
        if(!bAudioRecording && D->Phase==ERecoveryPhase::Countdown && D->MissionTime>-10)
        {UAudioMixerBlueprintLibrary::StartRecordingOutput(this,40);bAudioRecording=true;}
        if(bAudioRecording && D->MissionTime>=20)
        {
            UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,
                TEXT("LaunchMix"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Recovery/Audio")));
            bAudioRecording=false;bAudioRecorded=true;
        }
    }
    const double WallNow=FPlatformTime::Seconds();
    if(D->Phase==ERecoveryPhase::Ascent && D->MissionTime<20 && LastWallFrame>0)
    {
        FirstLaunchFrameMs.Add((WallNow-LastWallFrame)*1000);
        FirstLaunchPSOPeak=FMath::Max(FirstLaunchPSOPeak,int(FShaderPipelineCache::NumPrecompilesRemaining()));
    }
    LastWallFrame=WallNow;
    if(const auto* Acoustics=D->FindComponentByClass<URecoveryAudioComponent>())
        if(Acoustics->GetHeardPower()>.001)MaxAcousticDelayS=FMath::Max(MaxAcousticDelayS,Acoustics->GetDelayS());
    ++Samples;PeakAltitude=FMath::Max(PeakAltitude,D->AltitudeM);PeakSpeed=FMath::Max(PeakSpeed,D->VelocityMps.Size());
    if(D->AltitudeM>80000)++HighAltitudeSamples;
    if(Samples>5)
    {
        MaxCameraErrorCm=FMath::Max(MaxCameraErrorCm,FVector::Distance(PC->PlayerCameraManager->GetCameraLocation(),PC->GetViewTarget()->GetActorLocation()));
        MaxAngleErrorDeg=FMath::Max(MaxAngleErrorDeg,FMath::RadiansToDegrees(PC->PlayerCameraManager->GetCameraRotation().Quaternion().AngularDistance(PC->GetViewTarget()->GetActorQuat())));
    }
    bCaptured|=D->Phase==ERecoveryPhase::Captured;
    if(D->Phase==ERecoveryPhase::Captured && D->Viewer->GetCameraMode()==6)
    {
        const FVector Offset=PC->PlayerCameraManager->GetCameraLocation()-D->GetBody()->GetComponentLocation();
        const FQuat Rotation=PC->PlayerCameraManager->GetCameraRotation().Quaternion();
        if(ChaseContactFrames>0)
        {
            MaxChaseContactStepCm=FMath::Max(MaxChaseContactStepCm,(Offset-LastChaseOffset).Size());
            MaxChaseContactAngleDeg=FMath::Max(MaxChaseContactAngleDeg,FMath::RadiansToDegrees(Rotation.AngularDistance(LastChaseRotation)));
        }
        LastChaseOffset=Offset;LastChaseRotation=Rotation;++ChaseContactFrames;
    }
    if(const auto* Site=D->FindComponentByClass<URecoverySiteDetailsComponent>())SiteDetailInstances=FMath::Max(SiteDetailInstances,Site->GetInstanceCount());
    if(D->bSeparated)
    {
        int32 VisibleJets=0;TInlineComponentArray<UStaticMeshComponent*> Artwork(D);
        for(const auto* Part:Artwork)
        {
            if(Part->GetName()==TEXT("StarshipRenderBody") && Part->IsVisible())
            {
                ++StarshipFrames;
                MaxStarshipErrorCm=FMath::Max(MaxStarshipErrorCm,FVector::Distance(Part->GetComponentLocation(),D->GetUpperStageBaseTransform().GetLocation()));
            }
            if(Part->GetName().StartsWith(TEXT("StarshipPlume_")) && Part->IsVisible())++VisibleJets;
        }
        StarshipPlumes=FMath::Max(StarshipPlumes,VisibleJets);
    }
    if(const auto* V=D->FindComponentByClass<URecoveryVaporComponent>())
    {PeakVolumes=FMath::Max(PeakVolumes,V->GetActiveVolumeCount());bVaporHasDensity|=V->HasRenderableDensity();PeakTurbulentVolumes=FMath::Max(PeakTurbulentVolumes,V->GetTurbulentVolumeCount());}
    int Lights=0,Spots=0,Sounds=0;
    TInlineComponentArray<UPointLightComponent*> Points(D);
    for(const auto* L:Points)if(L->GetName().StartsWith(TEXT("PlumeLight_")))
    {++Lights;bVaporLit|=L->Intensity>0 && L->VolumetricScatteringIntensity>0;}
    TInlineComponentArray<USpotLightComponent*> Floods(D);
    for(const auto* L:Floods)if(L->IsRegistered())++Spots;
    TInlineComponentArray<UAudioComponent*> Audio(D);
    for(const auto* A:Audio)if(A->IsPlaying())++Sounds;
    PlumeLights=FMath::Max(PlumeLights,Lights);SiteLights=FMath::Max(SiteLights,Spots);PlayingAudio=FMath::Max(PlayingAudio,Sounds);
}
void URecoveryDiagnosticsComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bVisualReview)WriteVisualReview();
    if(bEnabled)
    {
        auto R=MakeShared<FJsonObject>();
        const auto* Director=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
        const auto* Acoustics=Director?Director->FindComponentByClass<URecoveryAudioComponent>():nullptr;
        const bool AudioReady=Acoustics && Acoustics->IsReady() && Acoustics->GetHistorySamples()>100;
        R->SetBoolField(TEXT("acoustics_ready"),AudioReady);
        R->SetNumberField(TEXT("mechanical_events_played"),Acoustics?Acoustics->GetPlayedMechanicalEvents():0);
        R->SetNumberField(TEXT("acoustic_history_samples"),Acoustics?Acoustics->GetHistorySamples():0);
        R->SetNumberField(TEXT("max_audible_propagation_delay_s"),MaxAcousticDelayS);
        FirstLaunchFrameMs.Sort();
        if(FirstLaunchFrameMs.Num())
        {
            R->SetNumberField(TEXT("first_launch_frame_p50_ms"),FirstLaunchFrameMs[FirstLaunchFrameMs.Num()/2]);
            R->SetNumberField(TEXT("first_launch_frame_p95_ms"),FirstLaunchFrameMs[FMath::Min(FirstLaunchFrameMs.Num()-1,int(FirstLaunchFrameMs.Num()*.95))]);
            R->SetNumberField(TEXT("first_launch_frame_max_ms"),FirstLaunchFrameMs.Last());
            R->SetNumberField(TEXT("first_launch_frames"),FirstLaunchFrameMs.Num());
            R->SetNumberField(TEXT("first_launch_pending_pso_peak"),FirstLaunchPSOPeak);
        }
        const auto* Fog=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"));
        const bool LitVolume=Fog && Fog->GetInt()==1;
        R->SetBoolField(TEXT("success"),bCaptured && HighAltitudeSamples>10 && MaxCameraErrorCm<.1 && MaxAngleErrorDeg<.01 && PeakVolumes>10 && PlumeLights==3 && SiteLights>=8 && PlayingAudio>=7 && PlayingAudio<=11 && AudioReady && Acoustics->GetPlayedMechanicalEvents()>=1 && bVaporLit && LitVolume && bVaporHasDensity && StarshipFrames>10 && MaxStarshipErrorCm<.1 && StarshipPlumes==6 && SiteDetailInstances>1000);
        R->SetNumberField(TEXT("frames"),Samples);R->SetNumberField(TEXT("frames_above_80_km"),HighAltitudeSamples);
        R->SetNumberField(TEXT("peak_altitude_m"),PeakAltitude);R->SetNumberField(TEXT("peak_speed_mps"),PeakSpeed);
        R->SetNumberField(TEXT("max_cached_camera_error_cm"),MaxCameraErrorCm);R->SetNumberField(TEXT("max_cached_camera_angle_error_deg"),MaxAngleErrorDeg);
        R->SetNumberField(TEXT("peak_active_vapor_volumes"),PeakVolumes);R->SetNumberField(TEXT("plume_lights"),PlumeLights);R->SetNumberField(TEXT("site_spotlights"),SiteLights);R->SetNumberField(TEXT("playing_audio_sources"),PlayingAudio);
        R->SetBoolField(TEXT("vapor_voxelized_with_local_lights"),LitVolume && bVaporLit);R->SetBoolField(TEXT("captured"),bCaptured);
        R->SetBoolField(TEXT("vapor_volume_material_has_density"),bVaporHasDensity);
        R->SetNumberField(TEXT("peak_turbulent_svt_volumes"),PeakTurbulentVolumes);
        R->SetBoolField(TEXT("turbulent_volume_budget_pass"),PeakTurbulentVolumes>0 && PeakTurbulentVolumes<=8);
        R->SetBoolField(TEXT("success"),R->GetBoolField(TEXT("success")) && PeakTurbulentVolumes>0 && PeakTurbulentVolumes<=8);
        R->SetNumberField(TEXT("starship_visible_physical_frames"),StarshipFrames);
        R->SetNumberField(TEXT("max_starship_render_pose_error_cm"),MaxStarshipErrorCm);
        R->SetNumberField(TEXT("starship_visible_exhausts"),StarshipPlumes);
        R->SetNumberField(TEXT("site_detail_instances"),SiteDetailInstances);
        R->SetNumberField(TEXT("chase_contact_frames"),ChaseContactFrames);
        R->SetNumberField(TEXT("chase_contact_max_offset_step_cm"),MaxChaseContactStepCm);
        R->SetNumberField(TEXT("chase_contact_max_angle_step_deg"),MaxChaseContactAngleDeg);
        if(FParse::Param(FCommandLine::Get(),TEXT("RecoveryChaseReview")))
            R->SetBoolField(TEXT("success"),R->GetBoolField(TEXT("success")) && ChaseContactFrames>30 && MaxChaseContactStepCm<10 && MaxChaseContactAngleDeg<.05);
        R->SetStringField(TEXT("reconstruction"),RecoveryRenderSettings::ActiveReconstruction());
        R->SetNumberField(TEXT("screen_percentage"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetFloat());
        R->SetNumberField(TEXT("fog_grid_pixel_size"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.GridPixelSize"))->GetInt());
        FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Recovery/experience-flight-audit.json")));
    }
    Super::EndPlay(Reason);
}

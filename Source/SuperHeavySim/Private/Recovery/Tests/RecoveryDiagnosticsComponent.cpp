#include "Recovery/Tests/RecoveryDiagnosticsComponent.h"
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

URecoveryDiagnosticsComponent::URecoveryDiagnosticsComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
void URecoveryDiagnosticsComponent::BeginPlay()
{
    Super::BeginPlay();
    bEnabled=FParse::Param(FCommandLine::Get(),TEXT("RecoveryExperienceAudit"));
    SetComponentTickEnabled(bEnabled);
}
void URecoveryDiagnosticsComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    const auto* PC=GetWorld()->GetFirstPlayerController();
    if(!D || !PC || !PC->PlayerCameraManager || !PC->GetViewTarget() || !D->GetBody())return;
    ++Samples;PeakAltitude=FMath::Max(PeakAltitude,D->AltitudeM);PeakSpeed=FMath::Max(PeakSpeed,D->VelocityMps.Size());
    if(D->AltitudeM>80000)++HighAltitudeSamples;
    if(Samples>5)
    {
        MaxCameraErrorCm=FMath::Max(MaxCameraErrorCm,FVector::Distance(PC->PlayerCameraManager->GetCameraLocation(),PC->GetViewTarget()->GetActorLocation()));
        MaxAngleErrorDeg=FMath::Max(MaxAngleErrorDeg,FMath::RadiansToDegrees(PC->PlayerCameraManager->GetCameraRotation().Quaternion().AngularDistance(PC->GetViewTarget()->GetActorQuat())));
    }
    bCaptured|=D->Phase==ERecoveryPhase::Captured;
    if(D->Phase==ERecoveryPhase::Captured && D->GetCameraMode()==6)
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
    {PeakVolumes=FMath::Max(PeakVolumes,V->GetActiveVolumeCount());bVaporHasDensity|=V->HasRenderableDensity();}
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
    if(bEnabled)
    {
        auto R=MakeShared<FJsonObject>();
        const auto* Fog=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"));
        const bool LitVolume=Fog && Fog->GetInt()==1;
        R->SetBoolField(TEXT("success"),bCaptured && HighAltitudeSamples>10 && MaxCameraErrorCm<.1 && MaxAngleErrorDeg<.01 && PeakVolumes>10 && PlumeLights==3 && SiteLights>=8 && PlayingAudio==2 && bVaporLit && LitVolume && bVaporHasDensity && StarshipFrames>10 && MaxStarshipErrorCm<.1 && StarshipPlumes==6 && SiteDetailInstances>1000);
        R->SetNumberField(TEXT("frames"),Samples);R->SetNumberField(TEXT("frames_above_80_km"),HighAltitudeSamples);
        R->SetNumberField(TEXT("peak_altitude_m"),PeakAltitude);R->SetNumberField(TEXT("peak_speed_mps"),PeakSpeed);
        R->SetNumberField(TEXT("max_cached_camera_error_cm"),MaxCameraErrorCm);R->SetNumberField(TEXT("max_cached_camera_angle_error_deg"),MaxAngleErrorDeg);
        R->SetNumberField(TEXT("peak_active_vapor_volumes"),PeakVolumes);R->SetNumberField(TEXT("plume_lights"),PlumeLights);R->SetNumberField(TEXT("site_spotlights"),SiteLights);R->SetNumberField(TEXT("playing_audio_sources"),PlayingAudio);
        R->SetBoolField(TEXT("vapor_voxelized_with_local_lights"),LitVolume && bVaporLit);R->SetBoolField(TEXT("captured"),bCaptured);
        R->SetBoolField(TEXT("vapor_volume_material_has_density"),bVaporHasDensity);
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

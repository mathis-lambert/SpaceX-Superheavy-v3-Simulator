#include "Recovery/Presentation/RecoverySkyComponent.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/VolumetricCloudComponent.h"
#include "EngineUtils.h"
#include "Misc/App.h"

URecoverySkyComponent::URecoverySkyComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
void URecoverySkyComponent::FindScene()
{
    for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It) if(!Sun.IsValid()) Sun=*It;
    for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It) if(It->bUnbound) { Exposure=*It;break; }
    for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It) { Fog=It->GetComponent();break; }
    for(TActorIterator<ASkyLight> It(GetWorld());It;++It) { Sky=It->GetLightComponent();break; }
    for(TActorIterator<AVolumetricCloud> It(GetWorld());It;++It) { Clouds=It->FindComponentByClass<UVolumetricCloudComponent>();break; }
    if(Sun.IsValid()) Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    if(Sky.IsValid()) { Sky->SetMobility(EComponentMobility::Movable);Sky->SetRealTimeCaptureEnabled(true); }
    if(Clouds.IsValid())
    {
        Clouds->TracingMaxDistanceMode=EVolumetricCloudTracingMaxDistanceMode::DistanceFromCloudLayerEntryPoint;
        Clouds->SetTracingStartMaxDistance(1800.f);
        Clouds->SetTracingMaxDistance(450.f);
    }
    Moon=GetWorld()->SpawnActor<ADirectionalLight>();
    Moon->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    auto* Lunar=Cast<UDirectionalLightComponent>(Moon->GetLightComponent());
    Lunar->SetAtmosphereSunLight(true);Lunar->SetAtmosphereSunLightIndex(1);
    Lunar->SetLightColor(FLinearColor(0.63f,0.73f,1.f));Lunar->SetCastShadows(false);
    for(int I=0;I<12;++I)
    {
        auto* L=NewObject<UPointLightComponent>(GetOwner());L->SetMobility(EComponentMobility::Movable);
        L->SetWorldLocation(FVector(-9000+(I%6)*4000,I<6?-10500:10000,600));
        L->SetIntensityUnits(ELightUnits::Candelas);L->SetAttenuationRadius(9000);L->SetSourceRadius(20);
        L->SetLightColor(FLinearColor(1.f,0.72f,0.43f));L->SetCastShadows(false);
        L->SetVolumetricScatteringIntensity(0.3f);L->RegisterComponent();GetOwner()->AddInstanceComponent(L);SiteLights.Add(L);
    }
    BuildSiteLighting();
    if(auto* StarMaterial=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_StarField))
    {
        StarField=NewObject<UStaticMeshComponent>(GetOwner(),TEXT("DistantStars"));
        StarField->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        StarField->SetMaterial(0,StarMaterial);StarField->SetMobility(EComponentMobility::Movable);
        StarField->SetCollisionEnabled(ECollisionEnabled::NoCollision);StarField->SetCastShadow(false);
        StarField->SetVisibleInRayTracing(false);StarField->SetAffectDistanceFieldLighting(false);
        StarField->SetWorldScale3D(FVector(40000000.));
        StarField->RegisterComponent();GetOwner()->AddInstanceComponent(StarField);
    }
    bFound=true;
}
void URecoverySkyComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);if(!FApp::CanEverRender())return;
    if(!bFound)FindScene();
    const auto* PC=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    const double Hour=PC?PC->TimeOfDay:17.;
    // Local solar time at Boca Chica, representative September declination.
    const double Latitude=FMath::DegreesToRadians(25.9973),Decl=FMath::DegreesToRadians(6.);
    const double H=FMath::DegreesToRadians((Hour-12)*15);
    const FVector ToSun(-FMath::Cos(Decl)*FMath::Sin(H),FMath::Cos(Latitude)*FMath::Sin(Decl)-FMath::Sin(Latitude)*FMath::Cos(Decl)*FMath::Cos(H),FMath::Sin(Latitude)*FMath::Sin(Decl)+FMath::Cos(Latitude)*FMath::Cos(Decl)*FMath::Cos(H));
    const double Elevation=FMath::RadiansToDegrees(FMath::Asin(ToSun.Z));
    const float Day=FMath::SmoothStep(-5.,5.,Elevation),HighSun=FMath::SmoothStep(0.,35.,Elevation);
    if(FMath::Abs(Hour-LastHour)>0.001)
    {
        if(Sun.IsValid()) { Sun->SetActorRotation((-ToSun).Rotation());Sun->GetLightComponent()->SetIntensity(110000*Day);Sun->GetLightComponent()->SetLightColor(FLinearColor::White);Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(Day>.01f?2:1); }
        if(Moon.IsValid()) { Moon->SetActorRotation(FRotator(-35,35,0));Moon->GetLightComponent()->SetIntensity(0.3f*(1-Day));Cast<UDirectionalLightComponent>(Moon->GetLightComponent())->SetForwardShadingPriority(Day>.01f?1:2); }
        LastHour=Hour;
    }
    UpdateSiteLighting(1-FMath::SmoothStep(-5.f,5.f,float(Elevation)));
    FVector CameraLocation=FVector::ZeroVector;
    if(PC && PC->GetViewTarget()) CameraLocation=PC->GetViewTarget()->GetActorLocation();
    const double Altitude=FlightGeometry::AltitudeM(CameraLocation);
    if(Clouds.IsValid())Clouds->SetTracingStartMaxDistance(FMath::Max(1800.,FMath::CeilToDouble(Altitude/100000.)*100.+2000.));
    if(StarField)StarField->SetWorldLocation(CameraLocation);
    // Retain full screen resolution near silhouettes; distant cloud layers need
    // fewer depth samples once the camera is far above their upper boundary.
    const float Samples=FMath::Lerp(2.f,.75f,FMath::SmoothStep(7000.,20000.,Altitude));
    if(Clouds.IsValid() && FMath::Abs(Samples-LastCloudSamples)>.04f)
    {
        Clouds->SetViewSampleCountScale(Samples);
        Clouds->SetShadowViewSampleCountScale(FMath::Min(1.5f,Samples));
        LastCloudSamples=Samples;
    }
    if(Fog.IsValid())
    {
        const float Near=1-FMath::SmoothStep(1500.,16000.,Altitude);
        Fog->SetFogDensity(0.003f*Near*(PC?PC->FogAmount:1.f));Fog->SetVisibility(Near>0.0001);
        Fog->SetVolumetricFog(true);Fog->SetVolumetricFogDistance(150000);
        Fog->SetVolumetricFogScatteringDistribution(0.3f);
    }
    if(Exposure.IsValid())
    {
        auto& S=Exposure->Settings;
        const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
        const float EnginePower=D && D->ActiveEngines>0?FMath::Clamp(float(D->Throttle),0.f,1.f):0.f;
        const float BaseEV=FMath::Lerp(6.5f,12.5f,Day)+HighSun*1.7f;
        // A night tracking camera stops down as the engines ignite. This retains
        // surface detail instead of whitening the whole pad with a daylight source.
        const float TargetEV=FMath::Max(BaseEV,FMath::Lerp(BaseEV,8.f,EnginePower*(1-Day)));
        ExposureEV=ExposureEV<0?TargetEV:FMath::FInterpTo(ExposureEV,TargetEV,Dt,2.5f);
        const float EV=ExposureEV;
        S.bOverride_AutoExposureMinBrightness=S.bOverride_AutoExposureMaxBrightness=true;
        S.AutoExposureMinBrightness=S.AutoExposureMaxBrightness=EV;
        S.bOverride_BloomIntensity=true;S.BloomIntensity=0.32f;
        S.bOverride_MotionBlurAmount=true;S.MotionBlurAmount=PC?PC->MotionBlur:0.25f;
        S.bOverride_MotionBlurMax=true;S.MotionBlurMax=2.f;
        S.bOverride_FilmGrainIntensity=true;S.FilmGrainIntensity=PC?PC->CameraGrain:0.12f;
        S.bOverride_VignetteIntensity=true;S.VignetteIntensity=0.15f;
        S.bOverride_DepthOfFieldEnabled=true;S.DepthOfFieldEnabled=PC && PC->bCameraDepthOfField;
        S.bOverride_DepthOfFieldFstop=true;S.DepthOfFieldFstop=8.f;
        S.bOverride_DepthOfFieldFocalDistance=true;
        const double Focus=D?(D->BasePositionM*100+FVector(0,0,5500)-CameraLocation).Size():50000;
        S.DepthOfFieldFocalDistance=FMath::Max(2000.,Focus);
        S.bOverride_DepthOfFieldSensorWidth=true;S.DepthOfFieldSensorWidth=36.f;
    }
}

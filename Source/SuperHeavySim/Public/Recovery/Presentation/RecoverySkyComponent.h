#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoverySkyComponent.generated.h"
class ADirectionalLight;
class APostProcessVolume;
class UExponentialHeightFogComponent;
class USkyLightComponent;
class UPointLightComponent;
class UVolumetricCloudComponent;
class USkyAtmosphereComponent;
class USpotLightComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Recovery),meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoverySkyComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoverySkyComponent();
    bool IsReady() const { return bFound && Sun.IsValid() && Exposure.IsValid(); }
    UFUNCTION(BlueprintPure,Category="Environment|Sun")
    static FVector CalculateSunDirection(double LatitudeDeg,double LongitudeDeg,double LocalHour,double UtcOffsetHours,int32 DayOfYear);
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
private:
    TWeakObjectPtr<ADirectionalLight> Sun;
    TWeakObjectPtr<ADirectionalLight> Moon;
    TWeakObjectPtr<APostProcessVolume> Exposure;
    TWeakObjectPtr<UExponentialHeightFogComponent> Fog;
    TWeakObjectPtr<USkyLightComponent> Sky;
    TWeakObjectPtr<UVolumetricCloudComponent> Clouds;
    TWeakObjectPtr<USkyAtmosphereComponent> Atmosphere;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> SiteLights;
    UPROPERTY(Transient) TArray<TObjectPtr<USpotLightComponent>> FloodLights;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> BeaconMaterials;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> StarField;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WeatherMaterial;
    float WeatherCoverage=.48f,WeatherHaze=1.f;
    bool bFound=false;
    float LastHour=-1;
    int32 LastSolarDay=-1;
    float LastUtcOffset=100.f;
    FVector2D OriginLatLon=FVector2D(25.9973,-97.1569);
    float LastCloudSamples=-1;
    float RegionalCloudShadowStrength=.35f;
    float ExposureEV=-1;
    void FindScene();
    void BuildSiteLighting();
    void UpdateSiteLighting(float Night);
};

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
class USpotLightComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Recovery),meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoverySkyComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoverySkyComponent();
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
private:
    TWeakObjectPtr<ADirectionalLight> Sun;
    TWeakObjectPtr<ADirectionalLight> Moon;
    TWeakObjectPtr<APostProcessVolume> Exposure;
    TWeakObjectPtr<UExponentialHeightFogComponent> Fog;
    TWeakObjectPtr<USkyLightComponent> Sky;
    TWeakObjectPtr<UVolumetricCloudComponent> Clouds;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> SiteLights;
    UPROPERTY(Transient) TArray<TObjectPtr<USpotLightComponent>> FloodLights;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> BeaconMaterials;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> StarField;
    bool bFound=false;
    float LastHour=-1;
    float LastCloudSamples=-1;
    float ExposureEV=-1;
    void FindScene();
    void BuildSiteLighting();
    void UpdateSiteLighting(float Night);
};

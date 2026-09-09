#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoverySiteActivityComponent.generated.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ASuperHeavyRecoveryDirector;
class UHeterogeneousVolumeComponent;

/** Bounded scenery animation, entirely separate from flight dynamics. */
UCLASS(ClassGroup=(Recovery))
class SUPERHEAVYSIM_API URecoverySiteActivityComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoverySiteActivityComponent();
    bool IsReady() const { return bBuilt; }
    void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
    int32 GetVehicleCount() const { return Trucks.Num(); }
    int32 GetVentCount() const { return Vents.Num(); }
    double GetTrafficDistanceM() const { return TrafficDistance; }
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Trucks;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Flags;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> ClothMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Beacons;
    UPROPERTY(Transient) TArray<TObjectPtr<UHeterogeneousVolumeComponent>> Vents;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> VentMaterials;
    TArray<FVector> Road;
    double ActivityTime=0,TrafficDistance=0;
    double TrafficSpeed=0;
    bool bBuilt=false;
    void Build(const FTransform& Site);
    void UpdateFacilityVents(const FTransform& Site,const FVector& Wind);
};

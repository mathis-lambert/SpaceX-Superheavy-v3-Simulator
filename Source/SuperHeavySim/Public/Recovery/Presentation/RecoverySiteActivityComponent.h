#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoverySiteActivityComponent.generated.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ASuperHeavyRecoveryDirector;

/** Bounded scenery animation, entirely separate from flight dynamics. */
UCLASS(ClassGroup=(Recovery))
class SUPERHEAVYSIM_API URecoverySiteActivityComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoverySiteActivityComponent();
    void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Trucks;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Flags;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> ClothMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Beacons;
    double ActivityTime=0,TrafficDistance=0;
    bool bBuilt=false;
    void Build(const FTransform& Site);
};

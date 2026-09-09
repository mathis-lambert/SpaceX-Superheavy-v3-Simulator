#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoverySiteDetailsComponent.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/** Modular, instanced industrial scenery. No flight forces or collision surfaces. */
UCLASS(ClassGroup=(Recovery),meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoverySiteDetailsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoverySiteDetailsComponent();
    bool IsReady() const { return !bEnabled || bBuilt; }
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Site details") bool bEnabled=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Site details",meta=(ClampMin="100",ClampMax="10000")) double DrawDistanceM=1800;
    int32 GetInstanceCount() const;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Batches;
    bool bBuilt=false;
    void Build(const FTransform& Site);
    void BuildServiceFacilities();
    void Box(int32 Material,const FVector& CentreM,const FVector& SizeM,const FQuat& Rotation=FQuat::Identity);
    void Pipe(int32 Material,const FVector& StartM,const FVector& EndM,double DiameterM);
    void Ring(int32 Material,const FVector& CentreM,double RadiusM,double DiameterM);
    void Ladder(const FVector& BottomM,double HeightM);
};

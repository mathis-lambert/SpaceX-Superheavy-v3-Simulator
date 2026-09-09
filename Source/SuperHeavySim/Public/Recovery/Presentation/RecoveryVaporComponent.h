#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryVaporComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UHeterogeneousVolumeComponent;
class ASuperHeavyRecoveryDirector;

/** Participating medium near the pad and plume, voxelized into the scene fog grid. */
UCLASS(ClassGroup=(Recovery), meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoveryVaporComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryVaporComponent();
    bool IsReady() const { return Volumes.Num()>0 && CryogenicVolumes.Num()==2 && TurbulentVolumes.Num()==8; }
    virtual void TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Fn) override;
    int32 GetActiveVolumeCount() const;
    bool HasRenderableDensity() const;
    int32 GetCryogenicVolumeCount() const;
    int32 GetTurbulentVolumeCount() const;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Vapor",meta=(ClampMin="64",ClampMax="256")) int32 VolumeBudget=128;
private:
    enum class EVaporKind:uint8 { Deluge,Trail };
    struct FBillow
    {
        FVector Position=FVector::ZeroVector, Velocity=FVector::ZeroVector;
        float Age=100, Life=22, RadiusM=4, GrowthMps=2, Density=1;
        FVector FlowAxis=FVector::ForwardVector,ShapeScale=FVector::OneVector;
        EVaporKind Kind=EVaporKind::Deluge;
        float Seed=0;
    };
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Volumes;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY(Transient) TArray<TObjectPtr<UHeterogeneousVolumeComponent>> CryogenicVolumes;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> CryogenicMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UHeterogeneousVolumeComponent>> TurbulentVolumes;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> TurbulentMaterials;
    TArray<FBillow> TurbulentBillows;
    FVector TurbulentFrameOffset=FVector::ZeroVector;
    double TurbulentVoxelM=.15625,TurbulentSpawnClock=0;
    uint32 TurbulentGeneration=0;
    int32 NextTurbulent=0;
    bool bCryogenicInitialized=false;
    FVector2D CryogenicStrength=FVector2D::ZeroVector;
    double FlowTime=0;
    TArray<FBillow> Billows;
    int32 Next=0;
    double DelugeSpawnClock=0;
    uint32 LastMissionGeneration=0;
    FVector LastTrailPosition=FVector::ZeroVector;
    void Build();
    void UpdateCryogenic(float Dt,const ASuperHeavyRecoveryDirector& Director);
    void UpdateTurbulent(float Dt,const ASuperHeavyRecoveryDirector& Director,double Delivered);
    void Spawn(const FVector& Position,const FVector& Velocity,float Life,float Radius,float Growth,float Density,EVaporKind Kind);
};

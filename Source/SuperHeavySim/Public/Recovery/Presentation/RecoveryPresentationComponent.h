#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryPresentationComponent.generated.h"
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UNiagaraComponent;
UCLASS(ClassGroup=(Recovery),meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoveryPresentationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryPresentationComponent();
    bool IsReady() const { return bBuilt && Plumes.Num()>0 && PlumeMaterials.Num()==Plumes.Num() && VaporTrail; }
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
private:
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> NozzleCores;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Plumes;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> PlumeMaterials;
    TArray<double> ExhaustEnvelopes;
    uint32 LastGeneration=MAX_uint32;
    TArray<int32> EngineIndices;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MixingPlume;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MixingMaterial;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> UpperStage;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> UpperStagePlumes;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> UpperStageFlameMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RcsPods;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RcsPlumes;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> RcsMaterials;
    TArray<double> RcsEnvelopes;
    TArray<FVector> RcsDirections;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> EngineLights;
    UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> VaporTrail;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> PlumeLights;
    bool bBuilt=false,bTrailWasRunning=false;
    double Clock=0;
    int LastLitEngineCount=-1;
    void Build();
    void SyncActuatorMeshes();
};

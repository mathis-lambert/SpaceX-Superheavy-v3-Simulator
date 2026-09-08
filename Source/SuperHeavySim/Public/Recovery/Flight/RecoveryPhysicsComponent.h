#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryUpperStageModel.h"
#include "RecoveryPhysicsComponent.generated.h"

class UPrimitiveComponent;
class FRecoveryPhysicsCallback;
struct FRecoveryDynamicsSetup;

/** Owns the solver model. Game-thread actors exchange commands and snapshots only. */
UCLASS()
class URecoveryPhysicsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryPhysicsComponent();
    static FRecoveryDynamicsConfiguration BuildConfiguration(const USuperHeavyRecoveryProfile& Profile);
    static FRecoveryGuidanceConfiguration BuildGuidanceConfiguration(const USuperHeavyRecoveryProfile& Profile);
    static FRecoveryUpperStageConfiguration BuildUpperStageConfiguration(const USuperHeavyRecoveryProfile& Profile);
    void InitializeMission(const FRecoveryGuidanceConfiguration& Configuration,const TArray<FRecoveryEngineState>& Geometry,
        double FuelKg,double RcsFuelKg,const FRecoveryUpperStageConfiguration& UpperStage,uint32 Generation);
    void Submit(UPrimitiveComponent& Body,UPrimitiveComponent* UpperStage,UPrimitiveComponent& LeftRail,
        UPrimitiveComponent& RightRail,const FRecoveryDynamicsCommand& Command);
    bool Consume(FRecoveryDynamicsState& State,FRecoveryGuidanceState& Guidance,FRecoveryUpperStageState& UpperStage);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
private:
    FRecoveryPhysicsCallback* Callback=nullptr;
    TSharedPtr<const FRecoveryDynamicsSetup,ESPMode::ThreadSafe> Setup;
};

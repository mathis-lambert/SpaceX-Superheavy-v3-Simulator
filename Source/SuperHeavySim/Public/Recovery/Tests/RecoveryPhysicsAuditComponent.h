#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryPhysicsAuditComponent.generated.h"

class FRecoveryCadenceCallback;

struct FRecoveryStepStatistics
{
    int64 Count=0;
    double TotalS=0,MinimumS=DBL_MAX,MaximumS=0;
    void Add(double Dt)
    {
        if(Dt<=0 || !FMath::IsFinite(Dt))return;
        ++Count;TotalS+=Dt;MinimumS=FMath::Min(MinimumS,Dt);MaximumS=FMath::Max(MaximumS,Dt);
    }
};

/** Opt-in measurement of actual solver callbacks versus game-frame exchanges.
 * The callback only exchanges value data and never touches game-thread objects. */
UCLASS()
class URecoveryPhysicsAuditComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryPhysicsAuditComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void RecordGameStep(double Dt) { if(bEnabled)GameSteps.Add(Dt); }
private:
    void ConsumePhysicsSteps();
    FRecoveryCadenceCallback* Callback=nullptr;
    bool bEnabled=false;
    FRecoveryStepStatistics PhysicsSteps,GameSteps;
};

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryForceDisplayComponent.generated.h"

/** Read-only display of sampled SI forces. Disabled in ordinary play. */
UCLASS(ClassGroup=(Recovery))
class SUPERHEAVYSIM_API URecoveryForceDisplayComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryForceDisplayComponent();
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
};

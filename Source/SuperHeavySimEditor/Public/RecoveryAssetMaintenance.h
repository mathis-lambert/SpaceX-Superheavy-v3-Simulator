#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RecoveryAssetMaintenance.generated.h"
class UBlueprint;
UCLASS()
class SUPERHEAVYSIMEDITOR_API URecoveryAssetMaintenance : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Removes only legacy input entry nodes. Actuator events and construction
    // graphs remain intact; runtime rendering calls those events explicitly.
    UFUNCTION(BlueprintCallable,Category="Recovery|Maintenance")
    static int32 RemoveLegacyInputEvents(UBlueprint* Blueprint);
};

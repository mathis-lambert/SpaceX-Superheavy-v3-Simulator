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
    // Use Unreal's registered reimport handler without replacing the asset identity.
    UFUNCTION(BlueprintCallable,Category="Recovery|Maintenance")
    static bool ReimportAsset(UObject* Asset, const FString& SourceFile);

    // Removes only legacy input entry nodes. Actuator events and construction
    // graphs remain intact; runtime rendering calls those events explicitly.
    UFUNCTION(BlueprintCallable,Category="Recovery|Maintenance")
    static int32 RemoveLegacyInputEvents(UBlueprint* Blueprint);
};

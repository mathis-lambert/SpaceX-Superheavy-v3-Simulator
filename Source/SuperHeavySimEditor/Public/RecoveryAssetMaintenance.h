#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RecoveryAssetMaintenance.generated.h"
UCLASS()
class SUPERHEAVYSIMEDITOR_API URecoveryAssetMaintenance : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Use Unreal's registered reimport handler without replacing the asset identity.
    UFUNCTION(BlueprintCallable,Category="Recovery|Maintenance")
    static bool ReimportAsset(UObject* Asset, const FString& SourceFile);


};

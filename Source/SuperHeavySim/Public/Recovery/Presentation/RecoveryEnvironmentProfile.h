#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RecoveryEnvironmentProfile.generated.h"

/** Reproducible, surveyed terrain placement; editable without touching the flight model. */
UCLASS(BlueprintType)
class SUPERHEAVYSIM_API URecoveryEnvironmentProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Geography") double EarthRadiusM=6371000.;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Geography") FVector2D OriginLatLon=FVector2D(25.9973,-97.1569);
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scenery") TArray<FTransform> Grass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scenery") TArray<FTransform> Rocks;
};

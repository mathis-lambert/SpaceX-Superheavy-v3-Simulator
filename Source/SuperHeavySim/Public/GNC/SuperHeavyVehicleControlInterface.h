#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GNC/SuperHeavyGncTypes.h"
#include "SuperHeavyVehicleControlInterface.generated.h"

UINTERFACE(BlueprintType)
class SUPERHEAVYSIM_API USuperHeavyVehicleControlInterface : public UInterface
{
	GENERATED_BODY()
};

class SUPERHEAVYSIM_API ISuperHeavyVehicleControlInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Super Heavy|Vehicle Control")
	void ApplyActuatorCommand(const FSuperHeavyActuatorCommand& Command);
};

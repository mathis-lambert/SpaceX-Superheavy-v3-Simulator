#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GNC/SuperHeavyGncTypes.h"
#include "GNC/SuperHeavyVehicleControlInterface.h"
#include "SuperHeavyVehicleActor.generated.h"

UCLASS(Blueprintable)
class SUPERHEAVYSIM_API ASuperHeavyVehicleActor : public AActor, public ISuperHeavyVehicleControlInterface
{
	GENERATED_BODY()

public:
	ASuperHeavyVehicleActor();

	virtual void ApplyActuatorCommand_Implementation(const FSuperHeavyActuatorCommand& Command) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	TArray<FName> OuterEngineIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	TArray<FName> InnerEngineIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	TArray<FName> CenterEngineIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	FName GridFinXPId = TEXT("GF_XP");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	FName GridFinXMId = TEXT("GF_XM");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	FName GridFinYMId = TEXT("GF_YM");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
	bool bWarnOnUnhandledActuatorCommands = true;

	UFUNCTION(BlueprintCallable, Category = "Super Heavy|Vehicle API")
	void ConfigureDefaultActuatorIds();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Super Heavy|Vehicle API")
	void SetEngineThrottleCommand(FName EngineId, double Throttle);
	virtual void SetEngineThrottleCommand_Implementation(FName EngineId, double Throttle);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Super Heavy|Vehicle API")
	void SetEngineGimbalCommand(FName EngineId, double PitchDeg, double RollDeg);
	virtual void SetEngineGimbalCommand_Implementation(FName EngineId, double PitchDeg, double RollDeg);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Super Heavy|Vehicle API")
	void SetGridFinAngleCommand(FName GridFinId, double AngleDeg);
	virtual void SetGridFinAngleCommand_Implementation(FName GridFinId, double AngleDeg);

protected:
	UPROPERTY(Transient)
	bool bWarnedUnhandledThrottleCommand = false;

	UPROPERTY(Transient)
	bool bWarnedUnhandledGimbalCommand = false;

	UPROPERTY(Transient)
	bool bWarnedUnhandledGridFinCommand = false;

	void ApplyThrottleToGroup(const TArray<FName>& EngineIds, double Throttle);
	void ApplyGimbalToGroup(const TArray<FName>& EngineIds, double PitchDeg, double RollDeg);
};

#pragma once

#include "CoreMinimal.h"
#include "SuperHeavyControlTypes.generated.h"

UENUM(BlueprintType)
enum class ESuperHeavyBodyAxis : uint8
{
	X UMETA(DisplayName = "Body X"),
	Y UMETA(DisplayName = "Body Y"),
	Z UMETA(DisplayName = "Body Z")
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyEngineGroupUsage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Groups")
	bool bOuterThrottleEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Groups")
	bool bInnerThrottleEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Groups")
	bool bCenterThrottleEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Groups")
	bool bInnerGimbalEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Groups")
	bool bCenterGimbalEnabled = true;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyEngineDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	FName EngineId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	double AzimuthDeg = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyEngineGroupConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	FName GroupName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	TArray<FSuperHeavyEngineDefinition> Engines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	bool bUseForThrottleControl = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	bool bUseForGimbalControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group", meta = (ClampMin = "0.0"))
	double MaxThrustPerEngineN = 2430000.0;

	double GetMaxThrustN() const
	{
		return bUseForThrottleControl ? MaxThrustPerEngineN * Engines.Num() : 0.0;
	}
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyActuatorLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits")
	double MinThrottle = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits")
	double MaxThrottle = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits")
	double MaxGimbalDeg = 15.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limits")
	double MaxGridFinDeg = 60.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyEngineActuatorCommand
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	FName EngineId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyThrottle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyGimbal = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double Throttle = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double GimbalPitchDeg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double GimbalRollDeg = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyActuatorCommand
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	TArray<FSuperHeavyEngineActuatorCommand> EngineCommands;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyGridFins = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double GridFinXPCommandDeg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double GridFinXMCommandDeg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double GridFinYMCommandDeg = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyCommandSaturation
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bEngineThrottleSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bEngineGimbalSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bGridFinSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bAnySaturated = false;
};

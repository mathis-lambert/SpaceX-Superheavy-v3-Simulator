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
struct SUPERHEAVYSIM_API FSuperHeavyEngineGroupConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	FName GroupName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	TArray<FName> EngineIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	bool bUseForThrottleControl = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group")
	bool bUseForGimbalControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Group", meta = (ClampMin = "0.0"))
	double MaxThrustPerEngineN = 2430000.0;

	double GetMaxThrustN() const
	{
		return bUseForThrottleControl ? MaxThrustPerEngineN * EngineIds.Num() : 0.0;
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
struct SUPERHEAVYSIM_API FSuperHeavyActuatorCommand
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyOuterThrottle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyInnerThrottle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyCenterThrottle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyInnerGimbal = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyCenterGimbal = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	bool bApplyGridFins = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double OuterThrottle = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double InnerThrottle = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double CenterThrottle = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double InnerGimbalPitchDeg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double InnerGimbalRollDeg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double CenterGimbalPitchDeg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command")
	double CenterGimbalRollDeg = 0.0;

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
	bool bOuterThrottleSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bInnerThrottleSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bCenterThrottleSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bInnerGimbalSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bCenterGimbalSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bGridFinSaturated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Saturation")
	bool bAnySaturated = false;
};

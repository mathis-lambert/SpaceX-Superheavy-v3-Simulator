#pragma once

#include "CoreMinimal.h"
#include "SuperHeavyGncTypes.generated.h"

UENUM(BlueprintType)
enum class ESuperHeavyGuidanceMode : uint8
{
	Disabled UMETA(DisplayName = "Disabled"),
	VerticalSpeedHold UMETA(DisplayName = "Vertical Speed Hold"),
	AltitudeHold UMETA(DisplayName = "Altitude Hold"),
	LandingTarget UMETA(DisplayName = "Landing Target")
};

UENUM(BlueprintType)
enum class ESuperHeavyFlightPhase : uint8
{
	Manual UMETA(DisplayName = "Manual"),
	Idle UMETA(DisplayName = "Idle"),
	Liftoff UMETA(DisplayName = "Liftoff"),
	Ascent UMETA(DisplayName = "Ascent"),
	Coast UMETA(DisplayName = "Coast"),
	Boostback UMETA(DisplayName = "Boostback"),
	Entry UMETA(DisplayName = "Entry"),
	LandingBurn UMETA(DisplayName = "Landing Burn"),
	Touchdown UMETA(DisplayName = "Touchdown"),
	Abort UMETA(DisplayName = "Abort")
};

UENUM(BlueprintType)
enum class ESuperHeavyBodyAxis : uint8
{
	X UMETA(DisplayName = "Body X"),
	Y UMETA(DisplayName = "Body Y"),
	Z UMETA(DisplayName = "Body Z")
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyVehicleState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	double TimeSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector LocationWorldCm = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector LocationWorldM = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector VelocityWorldMps = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector AngularVelocityDegPerSec = FVector::ZeroVector;

	FQuat RotationWorldQuat = FQuat::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector BodyForwardWorld = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector BodyRightWorld = FVector::RightVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector BodyUpWorld = FVector::UpVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FVector AngularVelocityBodyDegPerSec = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	double AltitudeM = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	double VerticalSpeedMps = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	double MassKg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	double EstimatedTotalThrustN = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	double EstimatedTWR = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyControlTargets
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	double TargetVerticalSpeedMps = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	double TargetAltitudeM = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	FRotator TargetWorldAttitudeDeg = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	FVector LandingTargetWorldM = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	double LandingTargetYawDeg = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyActuatorCommand
{
	GENERATED_BODY()

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
struct SUPERHEAVYSIM_API FSuperHeavyGncTelemetry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	ESuperHeavyFlightPhase FlightPhase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	ESuperHeavyGuidanceMode GuidanceMode = ESuperHeavyGuidanceMode::Disabled;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	bool bGncEnabled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	bool bAttitudeHoldEnabled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyVehicleState State;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyActuatorCommand LastCommand;
};

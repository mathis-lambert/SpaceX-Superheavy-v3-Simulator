#pragma once

#include "CoreMinimal.h"
#include "Control/SuperHeavyControlTypes.h"
#include "Control/SuperHeavyPidController.h"
#include "SuperHeavyAutopilotTypes.generated.h"

UENUM(BlueprintType)
enum class ESuperHeavyAutopilotMode : uint8
{
	Manual UMETA(DisplayName = "Manual"),
	Armed UMETA(DisplayName = "Armed"),
	Auto UMETA(DisplayName = "Auto"),
	Abort UMETA(DisplayName = "Abort"),
	Complete UMETA(DisplayName = "Complete")
};

UENUM(BlueprintType)
enum class ESuperHeavyFlightPhase : uint8
{
	Manual UMETA(DisplayName = "Manual"),
	GroundIdle UMETA(DisplayName = "Ground Idle"),
	Liftoff UMETA(DisplayName = "Liftoff"),
	Ascent UMETA(DisplayName = "Ascent"),
	MainEngineCutoff UMETA(DisplayName = "Main Engine Cutoff"),
	Coast UMETA(DisplayName = "Coast"),
	Boostback UMETA(DisplayName = "Boostback"),
	Entry UMETA(DisplayName = "Entry"),
	Approach UMETA(DisplayName = "Approach"),
	LandingBurn UMETA(DisplayName = "Landing Burn"),
	Touchdown UMETA(DisplayName = "Touchdown"),
	Abort UMETA(DisplayName = "Abort")
};

UENUM(BlueprintType)
enum class ESuperHeavyGuidanceMode : uint8
{
	Disabled UMETA(DisplayName = "Disabled"),
	VerticalAscent UMETA(DisplayName = "Vertical Ascent"),
	Coast UMETA(DisplayName = "Coast"),
	TargetApproach UMETA(DisplayName = "Target Approach"),
	LandingBurn UMETA(DisplayName = "Landing Burn"),
	TouchdownHold UMETA(DisplayName = "Touchdown Hold")
};

UENUM(BlueprintType)
enum class ESuperHeavyPhaseTransitionCondition : uint8
{
	ElapsedTime UMETA(DisplayName = "Elapsed Time"),
	AltitudeAbove UMETA(DisplayName = "Altitude Above"),
	AltitudeBelow UMETA(DisplayName = "Altitude Below"),
	SpeedBelow UMETA(DisplayName = "Speed Below"),
	VerticalSpeedBelow UMETA(DisplayName = "Vertical Speed Below"),
	VerticalVelocityBelow UMETA(DisplayName = "Vertical Velocity Below"),
	VerticalVelocityAbove UMETA(DisplayName = "Vertical Velocity Above"),
	HorizontalDistanceBelow UMETA(DisplayName = "Horizontal Distance Below"),
	DistanceToTargetBelow UMETA(DisplayName = "Distance To Target Below"),
	Touchdown UMETA(DisplayName = "Touchdown")
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyMissionTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Target")
	FTransform LaunchTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Target")
	FTransform LandingTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Target")
	FVector LandingTargetVelocityMps = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Target")
	double AltitudeReferenceWorldZCm = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyPhaseTransition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	ESuperHeavyFlightPhase TargetPhase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	ESuperHeavyPhaseTransitionCondition Condition = ESuperHeavyPhaseTransitionCondition::ElapsedTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	double Threshold = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	double SecondaryThreshold = 0.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyPhaseControlConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController VerticalPositionPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController VerticalVelocityPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController LateralPositionXPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController LateralPositionYPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController LateralVelocityXPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController LateralVelocityYPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController AttitudePitchPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController AttitudeRollPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPidController AttitudeYawPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance Shaping", meta = (ClampMin = "0.0", ClampMax = "85.0"))
	double MaxTargetTiltDeg = 25.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance Shaping", meta = (ClampMin = "0.0"))
	double MaxLateralAccelerationMps2 = 25.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance Shaping", meta = (ClampMin = "0.0"))
	double MaxVerticalAccelerationMps2 = 40.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance Shaping", meta = (ClampMin = "1.0"))
	double VerticalBrakingSafetyFactor = 1.25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance Shaping", meta = (ClampMin = "0.1"))
	double MinVerticalBrakingDistanceM = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimbal Mixing", meta = (ClampMin = "0.0"))
	double MaxYawGimbalMixDeg = 5.0;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyFlightPhaseConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	ESuperHeavyFlightPhase Phase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	ESuperHeavyGuidanceMode GuidanceMode = ESuperHeavyGuidanceMode::Disabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "1.0", ClampMax = "500.0"))
	double ControlRateHz = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	double TargetAltitudeM = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	FVector TargetVelocityWorldMps = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	FRotator TargetAttitudeWorldDeg = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	bool bUseMissionLandingTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	bool bUseMissionLaunchPositionXY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	FSuperHeavyPhaseControlConfig Control;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actuators")
	FSuperHeavyActuatorLimits ActuatorLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actuators")
	FSuperHeavyEngineGroupUsage EngineGroupUsage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transitions")
	TArray<FSuperHeavyPhaseTransition> Transitions;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyMissionValidationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	bool bIsValid = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	TArray<FString> Errors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	TArray<FString> Warnings;
};

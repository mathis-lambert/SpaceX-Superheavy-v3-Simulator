#pragma once

#include "Autopilot/SuperHeavyAutopilotTypes.h"
#include "Control/SuperHeavyControlTypes.h"
#include "Navigation/SuperHeavyNavigationState.h"
#include "SuperHeavyTelemetryTypes.generated.h"

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyAutopilotDebugState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	FVector PositionErrorM = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	FVector VelocityErrorMps = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	FVector DesiredAccelerationWorldMps2 = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	FVector AttitudeErrorBodyDeg = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	double AvailableThrustN = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	double RequiredThrustN = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	double EstimatedTWR = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	FSuperHeavyActuatorCommand RawCommand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot Debug")
	FSuperHeavyCommandSaturation Saturation;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyTelemetry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	ESuperHeavyAutopilotMode AutopilotMode = ESuperHeavyAutopilotMode::Manual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	ESuperHeavyFlightPhase FlightPhase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FName MissionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	double PhaseElapsedTimeSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyNavigationState Navigation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyActuatorCommand LastCommand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyAutopilotDebugState Debug;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GNC/SuperHeavyGncTypes.h"
#include "GNC/SuperHeavyPidController.h"
#include "SuperHeavyFlightPhaseProfile.generated.h"

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyFlightPhaseValidationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	bool bIsValid = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	TArray<FString> Errors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	TArray<FString> Warnings;
};

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyFlightPhaseConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	ESuperHeavyFlightPhase Phase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	bool bEnableGnc = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance")
	ESuperHeavyGuidanceMode GuidanceMode = ESuperHeavyGuidanceMode::Disabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance")
	bool bEnableAttitudeHold = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance", meta = (ClampMin = "1.0", ClampMax = "500.0"))
	double ControlRateHz = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance")
	FSuperHeavyControlTargets Targets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actuators")
	FSuperHeavyActuatorLimits ActuatorLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actuators")
	FSuperHeavyEngineGroupUsage EngineGroupUsage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bOverrideAltitudePid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (EditCondition = "bOverrideAltitudePid"))
	FSuperHeavyPidController AltitudePid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bOverrideVerticalSpeedPid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (EditCondition = "bOverrideVerticalSpeedPid"))
	FSuperHeavyPidController VerticalSpeedPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bOverrideAttitudePitchPid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (EditCondition = "bOverrideAttitudePitchPid"))
	FSuperHeavyPidController AttitudePitchPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bOverrideAttitudeRollPid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (EditCondition = "bOverrideAttitudeRollPid"))
	FSuperHeavyPidController AttitudeRollPid;
};

UCLASS(BlueprintType)
class SUPERHEAVYSIM_API USuperHeavyFlightPhaseProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Phases")
	TArray<FSuperHeavyFlightPhaseConfig> Phases;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Flight Phases")
	bool FindConfigForPhase(ESuperHeavyFlightPhase Phase, FSuperHeavyFlightPhaseConfig& OutConfig) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Flight Phases|Validation")
	FSuperHeavyFlightPhaseValidationResult ValidateProfile() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Flight Phases|Validation")
	bool IsPhaseConfigured(ESuperHeavyFlightPhase Phase) const;
};

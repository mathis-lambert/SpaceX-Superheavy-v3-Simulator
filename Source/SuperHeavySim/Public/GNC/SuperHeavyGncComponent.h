#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GNC/SuperHeavyFlightPhaseProfile.h"
#include "GNC/SuperHeavyGncTypes.h"
#include "GNC/SuperHeavyPidController.h"
#include "SuperHeavyGncComponent.generated.h"

UCLASS(ClassGroup = (SuperHeavy), meta = (BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API USuperHeavyGncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USuperHeavyGncComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GNC")
	bool bGncEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GNC")
	ESuperHeavyGuidanceMode GuidanceMode = ESuperHeavyGuidanceMode::Disabled;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Phases")
	ESuperHeavyFlightPhase CurrentFlightPhase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Phases")
	TObjectPtr<USuperHeavyFlightPhaseProfile> PhaseProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Phases|Validation")
	bool bValidatePhaseProfileOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GNC")
	bool bEnableAttitudeHold = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GNC", meta = (ClampMin = "1.0", ClampMax = "500.0"))
	double ControlRateHz = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	FName PhysicsComponentName = TEXT("COL_Body_Main");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	double AltitudeReferenceWorldZCm = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	double GravityMps2 = 9.81;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engines")
	FSuperHeavyEngineGroupConfig OuterEngines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engines")
	FSuperHeavyEngineGroupConfig InnerEngines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engines")
	FSuperHeavyEngineGroupConfig CenterEngines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actuators")
	FSuperHeavyActuatorLimits ActuatorLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targets")
	FSuperHeavyControlTargets Targets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FSuperHeavyPidController AltitudePid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FSuperHeavyPidController VerticalSpeedPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FSuperHeavyPidController AttitudePitchPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FSuperHeavyPidController AttitudeRollPid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	ESuperHeavyBodyAxis PitchControlBodyAxis = ESuperHeavyBodyAxis::Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	ESuperHeavyBodyAxis RollControlBodyAxis = ESuperHeavyBodyAxis::X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	double GimbalPitchCommandSign = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	double GimbalRollCommandSign = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Application")
	bool bApplyCommandsToVehicle = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyVehicleState LastState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyActuatorCommand LastCommand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyGncTelemetry LastTelemetry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight Phases|Validation")
	FSuperHeavyFlightPhaseValidationResult LastPhaseProfileValidation;

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetGncEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetGuidanceMode(ESuperHeavyGuidanceMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetVerticalSpeedTarget(double TargetVerticalSpeedMps);

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetAltitudeTarget(double TargetAltitudeM);

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetTargetWorldAttitude(FRotator TargetAttitudeDeg);

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetLandingTarget(FVector TargetWorldM, double TargetYawDeg);

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void SetControlTargets(const FSuperHeavyControlTargets& NewTargets);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GNC")
	FSuperHeavyControlTargets GetControlTargets() const { return Targets; }

	UFUNCTION(BlueprintCallable, Category = "GNC")
	void ResetControllers();

	UFUNCTION(BlueprintCallable, Category = "Flight Phases")
	FSuperHeavyFlightPhaseValidationResult SetPhaseProfile(USuperHeavyFlightPhaseProfile* NewPhaseProfile, bool bLogValidation = true);

	UFUNCTION(BlueprintCallable, Category = "Flight Phases")
	bool SetFlightPhase(ESuperHeavyFlightPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Flight Phases")
	void ApplyFlightPhaseConfig(const FSuperHeavyFlightPhaseConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Flight Phases|Validation")
	FSuperHeavyFlightPhaseValidationResult ValidatePhaseProfile(bool bLogResult = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyVehicleState GetLastState() const { return LastState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyActuatorCommand GetLastCommand() const { return LastCommand; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyGncTelemetry GetLastTelemetry() const { return LastTelemetry; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	ESuperHeavyFlightPhase GetCurrentFlightPhase() const { return CurrentFlightPhase; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> PhysicsComponent;

	double ControlAccumulatorSeconds = 0.0;
	FSuperHeavyPidController DefaultAltitudePid;
	FSuperHeavyPidController DefaultVerticalSpeedPid;
	FSuperHeavyPidController DefaultAttitudePitchPid;
	FSuperHeavyPidController DefaultAttitudeRollPid;
	bool bWarnedMissingVehicleControlInterface = false;

	void ConfigureDefaultEngineGroups();
	void CaptureDefaultPidSettings();
	void ResolvePhysicsComponent();
	void RunControlStep(double ControlDeltaTime);

	FSuperHeavyVehicleState CaptureState(double ControlDeltaTime) const;
	FSuperHeavyActuatorCommand ComputeCommand(const FSuperHeavyVehicleState& State, double ControlDeltaTime);
	FSuperHeavyActuatorCommand SanitizeActuatorCommand(const FSuperHeavyActuatorCommand& Command) const;
	double ComputeThrottleForVerticalSpeed(const FSuperHeavyVehicleState& State, double TargetVerticalSpeedMps, double ControlDeltaTime);
	void ApplyAttitudeHold(const FSuperHeavyVehicleState& State, double ControlDeltaTime, FSuperHeavyActuatorCommand& Command);
	void ApplyCommand(const FSuperHeavyActuatorCommand& Command);
	void UpdateTelemetry();
	void LogPhaseProfileValidation(const FSuperHeavyFlightPhaseValidationResult& ValidationResult) const;

	double GetAvailableThrottleThrustN() const;
	double EstimateCommandedThrustN(const FSuperHeavyActuatorCommand& Command) const;
	static double EstimateGroupThrustN(const FSuperHeavyEngineGroupConfig& Group, double Throttle);
	static double GetBodyAxisValue(const FVector& Vector, ESuperHeavyBodyAxis Axis);
	static FVector ComputeAttitudeErrorBodyDeg(const FQuat& CurrentWorldQuat, const FQuat& TargetWorldQuat);
};

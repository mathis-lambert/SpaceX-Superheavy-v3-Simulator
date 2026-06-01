#pragma once

#include "Autopilot/SuperHeavyMissionProfile.h"
#include "Components/ActorComponent.h"
#include "Control/SuperHeavyControlTypes.h"
#include "Navigation/SuperHeavyNavigationComponent.h"
#include "Telemetry/SuperHeavyTelemetryTypes.h"
#include "SuperHeavyAutopilotComponent.generated.h"

UCLASS(ClassGroup = (SuperHeavy), meta = (BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API USuperHeavyAutopilotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USuperHeavyAutopilotComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot")
	TObjectPtr<USuperHeavyMissionProfile> MissionProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot")
	bool bStartAutopilotOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot")
	bool bApplyCommandsToVehicle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot")
	FName NavigationComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot")
	double GravityMps2 = 9.81;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	ESuperHeavyBodyAxis PitchControlBodyAxis = ESuperHeavyBodyAxis::Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	ESuperHeavyBodyAxis RollControlBodyAxis = ESuperHeavyBodyAxis::X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	double GimbalPitchCommandSign = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	double GimbalRollCommandSign = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engines")
	FSuperHeavyEngineGroupConfig OuterEngines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engines")
	FSuperHeavyEngineGroupConfig InnerEngines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engines")
	FSuperHeavyEngineGroupConfig CenterEngines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot")
	ESuperHeavyAutopilotMode AutopilotMode = ESuperHeavyAutopilotMode::Manual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot")
	ESuperHeavyFlightPhase CurrentPhase = ESuperHeavyFlightPhase::Manual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot")
	double PhaseElapsedTimeSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyTelemetry LastTelemetry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyAutopilotDebugState LastDebugState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autopilot|Validation")
	FSuperHeavyMissionValidationResult LastMissionValidation;

	UFUNCTION(BlueprintCallable, Category = "Autopilot")
	FSuperHeavyMissionValidationResult SetMissionProfile(USuperHeavyMissionProfile* NewMissionProfile);

	UFUNCTION(BlueprintCallable, Category = "Autopilot")
	bool StartAutopilotMission();

	UFUNCTION(BlueprintCallable, Category = "Autopilot")
	void StopAutopilot();

	UFUNCTION(BlueprintCallable, Category = "Autopilot")
	void EnterManualMode();

	UFUNCTION(BlueprintCallable, Category = "Autopilot")
	bool AbortMission();

	UFUNCTION(BlueprintCallable, Category = "Autopilot")
	bool RestartMission();

	UFUNCTION(BlueprintCallable, Category = "Manual Control")
	void SetManualCommand(const FSuperHeavyActuatorCommand& Command);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyTelemetry GetTelemetry() const { return LastTelemetry; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyAutopilotDebugState GetDebugState() const { return LastDebugState; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<USuperHeavyNavigationComponent> NavigationComponent;

	FSuperHeavyFlightPhaseConfig CurrentPhaseConfig;
	FSuperHeavyActuatorCommand LastCommand;
	FSuperHeavyActuatorCommand ManualCommand;
	double ControlAccumulatorSeconds = 0.0;
	bool bWarnedMissingVehicleControlInterface = false;

	void ConfigureDefaultEngineGroups();
	void ResolveNavigationComponent();
	void ResetVehicleToMissionStart() const;
	bool SetFlightPhase(ESuperHeavyFlightPhase NewPhase);
	void ApplyPhaseConfig(const FSuperHeavyFlightPhaseConfig& PhaseConfig);
	void ResetControllers();
	void RunControlStep(double ControlDeltaTime);
	void EvaluatePhaseTransitions(const FSuperHeavyNavigationState& State);
	bool IsTransitionConditionMet(const FSuperHeavyPhaseTransition& Transition, const FSuperHeavyNavigationState& State) const;
	FSuperHeavyActuatorCommand ComputeAutopilotCommand(const FSuperHeavyNavigationState& State, double ControlDeltaTime);
	FSuperHeavyActuatorCommand ComputeVerticalAscentCommand(const FSuperHeavyNavigationState& State, double ControlDeltaTime);
	FSuperHeavyActuatorCommand ComputeLandingCommand(const FSuperHeavyNavigationState& State, double ControlDeltaTime);
	void ApplyAttitudeControl(const FSuperHeavyNavigationState& State, const FRotator& TargetAttitudeWorldDeg, double ControlDeltaTime, FSuperHeavyActuatorCommand& Command);
	void ApplyCommand(const FSuperHeavyActuatorCommand& Command);
	void UpdateTelemetry(const FSuperHeavyNavigationState& State);
	double GetAvailableThrottleThrustN() const;
	double EstimateCommandedThrustN(const FSuperHeavyActuatorCommand& Command) const;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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
	double GimbalPitchCommandSign = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	double GimbalRollCommandSign = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	FName SetEngineThrottleFunctionName = TEXT("SetEngineThrottle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	FName SetEngineGimbalFunctionName = TEXT("SetEngineGimbal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	FName SetGridFinAngleFunctionName = TEXT("SetGridFinAngle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command Mapping")
	bool bApplyCommandsToBlueprint = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyVehicleState LastState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
	FSuperHeavyActuatorCommand LastCommand;

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
	void ResetControllers();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyVehicleState GetLastState() const { return LastState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Telemetry")
	FSuperHeavyActuatorCommand GetLastCommand() const { return LastCommand; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> PhysicsComponent;

	double ControlAccumulatorSeconds = 0.0;

	void ConfigureDefaultEngineGroups();
	void ResolvePhysicsComponent();
	void RunControlStep(double ControlDeltaTime);

	FSuperHeavyVehicleState CaptureState(double ControlDeltaTime) const;
	FSuperHeavyActuatorCommand ComputeCommand(const FSuperHeavyVehicleState& State, double ControlDeltaTime);
	double ComputeThrottleForVerticalSpeed(const FSuperHeavyVehicleState& State, double TargetVerticalSpeedMps, double ControlDeltaTime);
	void ApplyAttitudeHold(const FSuperHeavyVehicleState& State, double ControlDeltaTime, FSuperHeavyActuatorCommand& Command);
	void ApplyCommand(const FSuperHeavyActuatorCommand& Command);

	double GetAvailableThrottleThrustN() const;
	void SetGroupThrottle(const FSuperHeavyEngineGroupConfig& Group, double Throttle);
	void SetGroupGimbal(const FSuperHeavyEngineGroupConfig& Group, double PitchDeg, double RollDeg);
	bool InvokeNameFloatFunction(FName FunctionName, FName Id, double Value) const;
	bool InvokeNameTwoFloatFunction(FName FunctionName, FName Id, double FirstValue, double SecondValue) const;
	static void SetInputParamsByType(UFunction* Function, void* Params, const FName* NameValue, const double* FirstValue, const double* SecondValue);
};

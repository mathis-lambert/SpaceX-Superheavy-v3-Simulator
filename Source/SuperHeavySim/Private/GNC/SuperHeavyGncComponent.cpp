#include "GNC/SuperHeavyGncComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GNC/SuperHeavyVehicleControlInterface.h"
#include "Logging/SuperHeavyLog.h"

USuperHeavyGncComponent::USuperHeavyGncComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	ConfigureDefaultEngineGroups();

	AltitudePid.Kp = 0.4;
	AltitudePid.Kd = 0.08;
	AltitudePid.bClampOutput = true;
	AltitudePid.OutputMin = -80.0;
	AltitudePid.OutputMax = 80.0;

	VerticalSpeedPid.Kp = 0.5;
	VerticalSpeedPid.Kd = 0.1;
	VerticalSpeedPid.bClampOutput = true;
	VerticalSpeedPid.OutputMin = -20.0;
	VerticalSpeedPid.OutputMax = 20.0;

	AttitudePitchPid.Kp = 0.05;
	AttitudePitchPid.Kd = 0.18;
	AttitudePitchPid.bClampOutput = true;
	AttitudePitchPid.OutputMin = -5.0;
	AttitudePitchPid.OutputMax = 5.0;

	AttitudeRollPid = AttitudePitchPid;

	CaptureDefaultPidSettings();
}

void USuperHeavyGncComponent::BeginPlay()
{
	Super::BeginPlay();
	CaptureDefaultPidSettings();
	ResolvePhysicsComponent();

	if (bValidatePhaseProfileOnBeginPlay)
	{
		ValidatePhaseProfile(true);
	}
}

void USuperHeavyGncComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PhysicsComponent)
	{
		ResolvePhysicsComponent();
	}

	if (!bGncEnabled || GuidanceMode == ESuperHeavyGuidanceMode::Disabled)
	{
		LastState = CaptureState(DeltaTime);
		UpdateTelemetry();
		return;
	}

	const double ControlStepSeconds = 1.0 / FMath::Max(ControlRateHz, 1.0);
	ControlAccumulatorSeconds += DeltaTime;

	int32 StepsRun = 0;
	while (ControlAccumulatorSeconds >= ControlStepSeconds && StepsRun < 5)
	{
		RunControlStep(ControlStepSeconds);
		ControlAccumulatorSeconds -= ControlStepSeconds;
		++StepsRun;
	}

	if (StepsRun == 5)
	{
		ControlAccumulatorSeconds = 0.0;
	}
}

void USuperHeavyGncComponent::SetGncEnabled(bool bEnabled)
{
	if (bGncEnabled != bEnabled)
	{
		ResetControllers();
		ControlAccumulatorSeconds = 0.0;
	}

	bGncEnabled = bEnabled;
}

void USuperHeavyGncComponent::SetGuidanceMode(ESuperHeavyGuidanceMode NewMode)
{
	if (GuidanceMode != NewMode)
	{
		ResetControllers();
		ControlAccumulatorSeconds = 0.0;
	}

	GuidanceMode = NewMode;
}

bool USuperHeavyGncComponent::SetFlightPhase(ESuperHeavyFlightPhase NewPhase)
{
	if (NewPhase == ESuperHeavyFlightPhase::Manual)
	{
		CurrentFlightPhase = NewPhase;
		SetGncEnabled(false);
		GuidanceMode = ESuperHeavyGuidanceMode::Disabled;
		bEnableAttitudeHold = false;
		UpdateTelemetry();
		return true;
	}

	if (!PhaseProfile)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyGncComponent: no phase profile assigned for flight phase %d."), static_cast<int32>(NewPhase));
		UpdateTelemetry();
		return false;
	}

	const FSuperHeavyFlightPhaseValidationResult ValidationResult = ValidatePhaseProfile(true);
	if (!ValidationResult.bIsValid)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyGncComponent: refusing to apply flight phase because the phase profile is invalid."));
		UpdateTelemetry();
		return false;
	}

	FSuperHeavyFlightPhaseConfig Config;
	if (!PhaseProfile->FindConfigForPhase(NewPhase, Config))
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyGncComponent: no config found for flight phase %d."), static_cast<int32>(NewPhase));
		UpdateTelemetry();
		return false;
	}

	ApplyFlightPhaseConfig(Config);
	return true;
}

void USuperHeavyGncComponent::ApplyFlightPhaseConfig(const FSuperHeavyFlightPhaseConfig& Config)
{
	CurrentFlightPhase = Config.Phase;
	SetGncEnabled(Config.bEnableGnc);
	SetGuidanceMode(Config.GuidanceMode);
	bEnableAttitudeHold = Config.bEnableAttitudeHold;
	ControlRateHz = FMath::Clamp(Config.ControlRateHz, 1.0, 500.0);
	Targets = Config.Targets;
	ActuatorLimits = Config.ActuatorLimits;

	OuterEngines.bUseForThrottleControl = Config.EngineGroupUsage.bOuterThrottleEnabled;
	OuterEngines.bUseForGimbalControl = false;
	InnerEngines.bUseForThrottleControl = Config.EngineGroupUsage.bInnerThrottleEnabled;
	InnerEngines.bUseForGimbalControl = Config.EngineGroupUsage.bInnerGimbalEnabled;
	CenterEngines.bUseForThrottleControl = Config.EngineGroupUsage.bCenterThrottleEnabled;
	CenterEngines.bUseForGimbalControl = Config.EngineGroupUsage.bCenterGimbalEnabled;

	AltitudePid = Config.bOverrideAltitudePid ? Config.AltitudePid : DefaultAltitudePid;
	VerticalSpeedPid = Config.bOverrideVerticalSpeedPid ? Config.VerticalSpeedPid : DefaultVerticalSpeedPid;
	AttitudePitchPid = Config.bOverrideAttitudePitchPid ? Config.AttitudePitchPid : DefaultAttitudePitchPid;
	AttitudeRollPid = Config.bOverrideAttitudeRollPid ? Config.AttitudeRollPid : DefaultAttitudeRollPid;

	ResetControllers();
	ControlAccumulatorSeconds = 0.0;
	UpdateTelemetry();
}

FSuperHeavyFlightPhaseValidationResult USuperHeavyGncComponent::ValidatePhaseProfile(bool bLogResult)
{
	LastPhaseProfileValidation = FSuperHeavyFlightPhaseValidationResult();

	if (!PhaseProfile)
	{
		LastPhaseProfileValidation.bIsValid = false;
		LastPhaseProfileValidation.Errors.Add(TEXT("No SuperHeavyFlightPhaseProfile is assigned."));
	}
	else
	{
		LastPhaseProfileValidation = PhaseProfile->ValidateProfile();
	}

	if (bLogResult)
	{
		LogPhaseProfileValidation(LastPhaseProfileValidation);
	}

	return LastPhaseProfileValidation;
}

void USuperHeavyGncComponent::SetVerticalSpeedTarget(double TargetVerticalSpeedMps)
{
	Targets.TargetVerticalSpeedMps = TargetVerticalSpeedMps;
}

void USuperHeavyGncComponent::SetAltitudeTarget(double TargetAltitudeM)
{
	Targets.TargetAltitudeM = TargetAltitudeM;
}

void USuperHeavyGncComponent::SetTargetWorldAttitude(FRotator TargetAttitudeDeg)
{
	Targets.TargetWorldAttitudeDeg = TargetAttitudeDeg;
}

void USuperHeavyGncComponent::SetLandingTarget(FVector TargetWorldM, double TargetYawDeg)
{
	Targets.LandingTargetWorldM = TargetWorldM;
	Targets.LandingTargetYawDeg = TargetYawDeg;
}

void USuperHeavyGncComponent::SetControlTargets(const FSuperHeavyControlTargets& NewTargets)
{
	Targets = NewTargets;
}

FSuperHeavyFlightPhaseValidationResult USuperHeavyGncComponent::SetPhaseProfile(USuperHeavyFlightPhaseProfile* NewPhaseProfile, bool bLogValidation)
{
	PhaseProfile = NewPhaseProfile;
	return ValidatePhaseProfile(bLogValidation);
}

void USuperHeavyGncComponent::ResetControllers()
{
	AltitudePid.Reset();
	VerticalSpeedPid.Reset();
	AttitudePitchPid.Reset();
	AttitudeRollPid.Reset();
}

void USuperHeavyGncComponent::ConfigureDefaultEngineGroups()
{
	OuterEngines.GroupName = TEXT("Outer");
	OuterEngines.bUseForThrottleControl = true;
	OuterEngines.bUseForGimbalControl = false;
	OuterEngines.EngineIds.Reset();
	for (int32 Index = 1; Index <= 20; ++Index)
	{
		OuterEngines.EngineIds.Add(FName(*FString::Printf(TEXT("R%02d"), Index)));
	}

	InnerEngines.GroupName = TEXT("Inner");
	InnerEngines.bUseForThrottleControl = true;
	InnerEngines.bUseForGimbalControl = true;
	InnerEngines.EngineIds.Reset();
	for (int32 Index = 1; Index <= 10; ++Index)
	{
		InnerEngines.EngineIds.Add(FName(*FString::Printf(TEXT("RGI%02d"), Index)));
	}

	CenterEngines.GroupName = TEXT("Center");
	CenterEngines.bUseForThrottleControl = true;
	CenterEngines.bUseForGimbalControl = true;
	CenterEngines.EngineIds = { TEXT("RGC01"), TEXT("RGC02"), TEXT("RGC03") };
}

void USuperHeavyGncComponent::CaptureDefaultPidSettings()
{
	DefaultAltitudePid = AltitudePid;
	DefaultVerticalSpeedPid = VerticalSpeedPid;
	DefaultAttitudePitchPid = AttitudePitchPid;
	DefaultAttitudeRollPid = AttitudeRollPid;

	DefaultAltitudePid.Reset();
	DefaultVerticalSpeedPid.Reset();
	DefaultAttitudePitchPid.Reset();
	DefaultAttitudeRollPid.Reset();
}

void USuperHeavyGncComponent::ResolvePhysicsComponent()
{
	PhysicsComponent = nullptr;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (Component && Component->GetFName() == PhysicsComponentName)
		{
			PhysicsComponent = Component;
			return;
		}
	}

	PhysicsComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
}

void USuperHeavyGncComponent::RunControlStep(double ControlDeltaTime)
{
	LastState = CaptureState(ControlDeltaTime);
	LastCommand = SanitizeActuatorCommand(ComputeCommand(LastState, ControlDeltaTime));
	LastState.EstimatedTotalThrustN = EstimateCommandedThrustN(LastCommand);
	LastState.EstimatedTWR = LastState.MassKg > UE_SMALL_NUMBER
		? LastState.EstimatedTotalThrustN / (LastState.MassKg * GravityMps2)
		: 0.0;

	if (bApplyCommandsToVehicle)
	{
		ApplyCommand(LastCommand);
	}

	UpdateTelemetry();
}

FSuperHeavyVehicleState USuperHeavyGncComponent::CaptureState(double ControlDeltaTime) const
{
	FSuperHeavyVehicleState State;
	State.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	UPrimitiveComponent* Body = PhysicsComponent.Get();
	if (!Body)
	{
		return State;
	}

	State.LocationWorldCm = Body->GetComponentLocation();
	State.LocationWorldM = State.LocationWorldCm / 100.0;
	State.VelocityWorldMps = Body->GetPhysicsLinearVelocity() / 100.0;
	State.AngularVelocityDegPerSec = Body->GetPhysicsAngularVelocityInDegrees();
	State.RotationWorldQuat = Body->GetComponentQuat();
	State.BodyForwardWorld = Body->GetForwardVector();
	State.BodyRightWorld = Body->GetRightVector();
	State.BodyUpWorld = Body->GetUpVector();
	State.AngularVelocityBodyDegPerSec = State.RotationWorldQuat.Inverse().RotateVector(State.AngularVelocityDegPerSec);
	State.AltitudeM = (State.LocationWorldCm.Z - AltitudeReferenceWorldZCm) / 100.0;
	State.VerticalSpeedMps = State.VelocityWorldMps.Z;
	State.MassKg = Body->GetMass();
	State.EstimatedTotalThrustN = EstimateCommandedThrustN(LastCommand);
	State.EstimatedTWR = State.MassKg > UE_SMALL_NUMBER
		? State.EstimatedTotalThrustN / (State.MassKg * GravityMps2)
		: 0.0;

	return State;
}

FSuperHeavyActuatorCommand USuperHeavyGncComponent::ComputeCommand(const FSuperHeavyVehicleState& State, double ControlDeltaTime)
{
	FSuperHeavyActuatorCommand Command;

	double CollectiveThrottle = 0.0;
	switch (GuidanceMode)
	{
	case ESuperHeavyGuidanceMode::VerticalSpeedHold:
		CollectiveThrottle = ComputeThrottleForVerticalSpeed(State, Targets.TargetVerticalSpeedMps, ControlDeltaTime);
		break;
	case ESuperHeavyGuidanceMode::AltitudeHold:
	{
		const double AltitudeErrorM = Targets.TargetAltitudeM - State.AltitudeM;
		const double TargetVerticalSpeedMps = AltitudePid.Update(AltitudeErrorM, ControlDeltaTime);
		CollectiveThrottle = ComputeThrottleForVerticalSpeed(State, TargetVerticalSpeedMps, ControlDeltaTime);
		break;
	}
	case ESuperHeavyGuidanceMode::LandingTarget:
	{
		const double TargetAltitudeM = Targets.LandingTargetWorldM.Z;
		const double AltitudeErrorM = TargetAltitudeM - State.LocationWorldM.Z;
		const double TargetVerticalSpeedMps = AltitudePid.Update(AltitudeErrorM, ControlDeltaTime);
		CollectiveThrottle = ComputeThrottleForVerticalSpeed(State, TargetVerticalSpeedMps, ControlDeltaTime);
		break;
	}
	default:
		break;
	}

	Command.OuterThrottle = OuterEngines.bUseForThrottleControl ? CollectiveThrottle : 0.0;
	Command.InnerThrottle = InnerEngines.bUseForThrottleControl ? CollectiveThrottle : 0.0;
	Command.CenterThrottle = CenterEngines.bUseForThrottleControl ? CollectiveThrottle : 0.0;

	if (bEnableAttitudeHold)
	{
		ApplyAttitudeHold(State, ControlDeltaTime, Command);
	}

	return Command;
}

FSuperHeavyActuatorCommand USuperHeavyGncComponent::SanitizeActuatorCommand(const FSuperHeavyActuatorCommand& Command) const
{
	FSuperHeavyActuatorCommand Sanitized = Command;

	Sanitized.OuterThrottle = OuterEngines.bUseForThrottleControl
		? FMath::Clamp(Sanitized.OuterThrottle, ActuatorLimits.MinThrottle, ActuatorLimits.MaxThrottle)
		: 0.0;
	Sanitized.InnerThrottle = InnerEngines.bUseForThrottleControl
		? FMath::Clamp(Sanitized.InnerThrottle, ActuatorLimits.MinThrottle, ActuatorLimits.MaxThrottle)
		: 0.0;
	Sanitized.CenterThrottle = CenterEngines.bUseForThrottleControl
		? FMath::Clamp(Sanitized.CenterThrottle, ActuatorLimits.MinThrottle, ActuatorLimits.MaxThrottle)
		: 0.0;

	Sanitized.InnerGimbalPitchDeg = InnerEngines.bUseForGimbalControl
		? FMath::Clamp(Sanitized.InnerGimbalPitchDeg, -ActuatorLimits.MaxGimbalDeg, ActuatorLimits.MaxGimbalDeg)
		: 0.0;
	Sanitized.InnerGimbalRollDeg = InnerEngines.bUseForGimbalControl
		? FMath::Clamp(Sanitized.InnerGimbalRollDeg, -ActuatorLimits.MaxGimbalDeg, ActuatorLimits.MaxGimbalDeg)
		: 0.0;
	Sanitized.CenterGimbalPitchDeg = CenterEngines.bUseForGimbalControl
		? FMath::Clamp(Sanitized.CenterGimbalPitchDeg, -ActuatorLimits.MaxGimbalDeg, ActuatorLimits.MaxGimbalDeg)
		: 0.0;
	Sanitized.CenterGimbalRollDeg = CenterEngines.bUseForGimbalControl
		? FMath::Clamp(Sanitized.CenterGimbalRollDeg, -ActuatorLimits.MaxGimbalDeg, ActuatorLimits.MaxGimbalDeg)
		: 0.0;

	Sanitized.GridFinXPCommandDeg = FMath::Clamp(Sanitized.GridFinXPCommandDeg, -ActuatorLimits.MaxGridFinDeg, ActuatorLimits.MaxGridFinDeg);
	Sanitized.GridFinXMCommandDeg = FMath::Clamp(Sanitized.GridFinXMCommandDeg, -ActuatorLimits.MaxGridFinDeg, ActuatorLimits.MaxGridFinDeg);
	Sanitized.GridFinYMCommandDeg = FMath::Clamp(Sanitized.GridFinYMCommandDeg, -ActuatorLimits.MaxGridFinDeg, ActuatorLimits.MaxGridFinDeg);

	return Sanitized;
}

double USuperHeavyGncComponent::ComputeThrottleForVerticalSpeed(const FSuperHeavyVehicleState& State, double TargetVerticalSpeedMps, double ControlDeltaTime)
{
	const double AvailableThrustN = GetAvailableThrottleThrustN();
	if (AvailableThrustN <= UE_SMALL_NUMBER || State.MassKg <= UE_SMALL_NUMBER)
	{
		return 0.0;
	}

	const double VerticalSpeedErrorMps = TargetVerticalSpeedMps - State.VerticalSpeedMps;
	const double DesiredAccelerationMps2 = VerticalSpeedPid.Update(VerticalSpeedErrorMps, ControlDeltaTime);
	const double UpAlignment = FMath::Clamp(FVector::DotProduct(State.BodyUpWorld.GetSafeNormal(), FVector::UpVector), 0.2, 1.0);
	const double RequiredThrustN = State.MassKg * (GravityMps2 + DesiredAccelerationMps2) / UpAlignment;
	const double RawThrottle = RequiredThrustN / AvailableThrustN;

	return FMath::Clamp(RawThrottle, ActuatorLimits.MinThrottle, ActuatorLimits.MaxThrottle);
}

void USuperHeavyGncComponent::ApplyAttitudeHold(const FSuperHeavyVehicleState& State, double ControlDeltaTime, FSuperHeavyActuatorCommand& Command)
{
	const FQuat TargetWorldQuat = Targets.TargetWorldAttitudeDeg.Quaternion();
	const FVector AttitudeErrorBodyDeg = ComputeAttitudeErrorBodyDeg(State.RotationWorldQuat, TargetWorldQuat);

	const double PitchErrorDeg = GetBodyAxisValue(AttitudeErrorBodyDeg, PitchControlBodyAxis);
	const double RollErrorDeg = GetBodyAxisValue(AttitudeErrorBodyDeg, RollControlBodyAxis);
	const double PitchRateDegPerSec = GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, PitchControlBodyAxis);
	const double RollRateDegPerSec = GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, RollControlBodyAxis);

	const double PitchCommandDeg = FMath::Clamp(
		AttitudePitchPid.UpdateWithMeasuredRate(PitchErrorDeg, PitchRateDegPerSec, ControlDeltaTime) * GimbalPitchCommandSign,
		-ActuatorLimits.MaxGimbalDeg,
		ActuatorLimits.MaxGimbalDeg);

	const double RollCommandDeg = FMath::Clamp(
		AttitudeRollPid.UpdateWithMeasuredRate(RollErrorDeg, RollRateDegPerSec, ControlDeltaTime) * GimbalRollCommandSign,
		-ActuatorLimits.MaxGimbalDeg,
		ActuatorLimits.MaxGimbalDeg);

	Command.CenterGimbalPitchDeg = CenterEngines.bUseForGimbalControl ? PitchCommandDeg : 0.0;
	Command.CenterGimbalRollDeg = CenterEngines.bUseForGimbalControl ? RollCommandDeg : 0.0;
	Command.InnerGimbalPitchDeg = InnerEngines.bUseForGimbalControl ? PitchCommandDeg : 0.0;
	Command.InnerGimbalRollDeg = InnerEngines.bUseForGimbalControl ? RollCommandDeg : 0.0;
}

void USuperHeavyGncComponent::ApplyCommand(const FSuperHeavyActuatorCommand& Command)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetClass()->ImplementsInterface(USuperHeavyVehicleControlInterface::StaticClass()))
	{
		if (!bWarnedMissingVehicleControlInterface)
		{
			UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyGncComponent: owner must implement SuperHeavyVehicleControlInterface to receive actuator commands."));
			bWarnedMissingVehicleControlInterface = true;
		}
		return;
	}

	ISuperHeavyVehicleControlInterface::Execute_ApplyActuatorCommand(Owner, Command);
}

void USuperHeavyGncComponent::UpdateTelemetry()
{
	LastTelemetry.FlightPhase = CurrentFlightPhase;
	LastTelemetry.GuidanceMode = GuidanceMode;
	LastTelemetry.bGncEnabled = bGncEnabled;
	LastTelemetry.bAttitudeHoldEnabled = bEnableAttitudeHold;
	LastTelemetry.State = LastState;
	LastTelemetry.LastCommand = LastCommand;
}

void USuperHeavyGncComponent::LogPhaseProfileValidation(const FSuperHeavyFlightPhaseValidationResult& ValidationResult) const
{
	for (const FString& Error : ValidationResult.Errors)
	{
		UE_LOG(LogSuperHeavyGnc, Error, TEXT("SuperHeavyGncComponent phase profile error: %s"), *Error);
	}

	for (const FString& Warning : ValidationResult.Warnings)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyGncComponent phase profile warning: %s"), *Warning);
	}
}

double USuperHeavyGncComponent::GetAvailableThrottleThrustN() const
{
	return OuterEngines.GetMaxThrustN() + InnerEngines.GetMaxThrustN() + CenterEngines.GetMaxThrustN();
}

double USuperHeavyGncComponent::EstimateCommandedThrustN(const FSuperHeavyActuatorCommand& Command) const
{
	return EstimateGroupThrustN(OuterEngines, Command.OuterThrottle)
		+ EstimateGroupThrustN(InnerEngines, Command.InnerThrottle)
		+ EstimateGroupThrustN(CenterEngines, Command.CenterThrottle);
}

double USuperHeavyGncComponent::EstimateGroupThrustN(const FSuperHeavyEngineGroupConfig& Group, double Throttle)
{
	return Group.GetMaxThrustN() * FMath::Clamp(Throttle, 0.0, 1.0);
}

double USuperHeavyGncComponent::GetBodyAxisValue(const FVector& Vector, ESuperHeavyBodyAxis Axis)
{
	switch (Axis)
	{
	case ESuperHeavyBodyAxis::X:
		return Vector.X;
	case ESuperHeavyBodyAxis::Y:
		return Vector.Y;
	case ESuperHeavyBodyAxis::Z:
		return Vector.Z;
	default:
		return 0.0;
	}
}

FVector USuperHeavyGncComponent::ComputeAttitudeErrorBodyDeg(const FQuat& CurrentWorldQuat, const FQuat& TargetWorldQuat)
{
	FQuat ErrorWorldQuat = TargetWorldQuat * CurrentWorldQuat.Inverse();
	ErrorWorldQuat.Normalize();

	if (ErrorWorldQuat.W < 0.0)
	{
		ErrorWorldQuat.X *= -1.0;
		ErrorWorldQuat.Y *= -1.0;
		ErrorWorldQuat.Z *= -1.0;
		ErrorWorldQuat.W *= -1.0;
	}

	FVector ErrorAxisWorld = FVector::ZeroVector;
	double ErrorAngleRad = 0.0;
	ErrorWorldQuat.ToAxisAndAngle(ErrorAxisWorld, ErrorAngleRad);

	if (!ErrorAxisWorld.IsNormalized())
	{
		ErrorAxisWorld = ErrorAxisWorld.GetSafeNormal();
	}

	const FVector ErrorWorldDeg = ErrorAxisWorld * FMath::RadiansToDegrees(ErrorAngleRad);
	return CurrentWorldQuat.Inverse().RotateVector(ErrorWorldDeg);
}

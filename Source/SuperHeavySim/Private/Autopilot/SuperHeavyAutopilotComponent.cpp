#include "Autopilot/SuperHeavyAutopilotComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Control/SuperHeavyActuatorCommandUtils.h"
#include "Control/SuperHeavyControlMath.h"
#include "GameFramework/Actor.h"
#include "Logging/SuperHeavyLog.h"
#include "Vehicle/SuperHeavyVehicleControlInterface.h"

USuperHeavyAutopilotComponent::USuperHeavyAutopilotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ConfigureDefaultEngineGroups();
}

void USuperHeavyAutopilotComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveNavigationComponent();

	if (MissionProfile)
	{
		LastMissionValidation = MissionProfile->ValidateMission();
	}

	if (bStartAutopilotOnBeginPlay)
	{
		StartAutopilotMission();
	}
}

void USuperHeavyAutopilotComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!NavigationComponent)
	{
		ResolveNavigationComponent();
	}

	if (!NavigationComponent)
	{
		return;
	}

	const FSuperHeavyMissionTarget MissionTarget = MissionProfile ? MissionProfile->Target : FSuperHeavyMissionTarget();
	const FSuperHeavyNavigationState State = NavigationComponent->CaptureNavigationState(MissionTarget.LandingTransform, MissionTarget.LandingTargetVelocityMps);

	if (AutopilotMode == ESuperHeavyAutopilotMode::Manual)
	{
		LastCommand = ManualCommand;
		if (bApplyCommandsToVehicle)
		{
			ApplyCommand(LastCommand);
		}
		UpdateTelemetry(State);
		return;
	}

	const double ControlStepSeconds = 1.0 / FMath::Max(CurrentPhaseConfig.ControlRateHz, 1.0);
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

FSuperHeavyMissionValidationResult USuperHeavyAutopilotComponent::SetMissionProfile(USuperHeavyMissionProfile* NewMissionProfile)
{
	MissionProfile = NewMissionProfile;
	LastMissionValidation = MissionProfile ? MissionProfile->ValidateMission() : FSuperHeavyMissionValidationResult();

	if (!MissionProfile)
	{
		LastMissionValidation.bIsValid = false;
		LastMissionValidation.Errors.Add(TEXT("No mission profile assigned."));
	}

	return LastMissionValidation;
}

bool USuperHeavyAutopilotComponent::StartAutopilotMission()
{
	if (!MissionProfile)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyAutopilotComponent: no mission profile assigned."));
		return false;
	}

	LastMissionValidation = MissionProfile->ValidateMission();
	if (!LastMissionValidation.bIsValid)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyAutopilotComponent: refusing to start invalid mission."));
		return false;
	}

	if (NavigationComponent)
	{
		NavigationComponent->AltitudeReferenceWorldZCm = MissionProfile->Target.AltitudeReferenceWorldZCm;
		NavigationComponent->ResetNavigation();
	}

	ResetVehicleToMissionStart();

	AutopilotMode = ESuperHeavyAutopilotMode::Auto;
	return SetFlightPhase(MissionProfile->InitialPhase);
}

void USuperHeavyAutopilotComponent::StopAutopilot()
{
	AutopilotMode = ESuperHeavyAutopilotMode::Manual;
	CurrentPhase = ESuperHeavyFlightPhase::Manual;
	CurrentPhaseConfig = FSuperHeavyFlightPhaseConfig();
	PhaseElapsedTimeSeconds = 0.0;
	ControlAccumulatorSeconds = 0.0;
	ResetControllers();
}

void USuperHeavyAutopilotComponent::EnterManualMode()
{
	StopAutopilot();
}

bool USuperHeavyAutopilotComponent::AbortMission()
{
	if (!MissionProfile)
	{
		StopAutopilot();
		return false;
	}

	AutopilotMode = ESuperHeavyAutopilotMode::Abort;
	return SetFlightPhase(MissionProfile->AbortPhase);
}

bool USuperHeavyAutopilotComponent::RestartMission()
{
	StopAutopilot();
	return StartAutopilotMission();
}

void USuperHeavyAutopilotComponent::SetManualCommand(const FSuperHeavyActuatorCommand& Command)
{
	ManualCommand = Command;
}

void USuperHeavyAutopilotComponent::ConfigureDefaultEngineGroups()
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

void USuperHeavyAutopilotComponent::ResetVehicleToMissionStart() const
{
	if (!MissionProfile || !MissionProfile->bResetVehicleToLaunchTransformOnStart || !NavigationComponent)
	{
		return;
	}

	UPrimitiveComponent* PhysicsComponent = NavigationComponent->GetPhysicsComponent();
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (PhysicsComponent && PhysicsComponent->IsSimulatingPhysics())
	{
		PhysicsComponent->SetWorldTransform(MissionProfile->Target.LaunchTransform, false, nullptr, ETeleportType::TeleportPhysics);
		PhysicsComponent->SetPhysicsLinearVelocity(MissionProfile->InitialLinearVelocityMps * 100.0);
		PhysicsComponent->SetPhysicsAngularVelocityInDegrees(MissionProfile->InitialAngularVelocityDegPerSec);
		return;
	}

	Owner->SetActorTransform(MissionProfile->Target.LaunchTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (PhysicsComponent)
	{
		PhysicsComponent->SetPhysicsLinearVelocity(MissionProfile->InitialLinearVelocityMps * 100.0);
		PhysicsComponent->SetPhysicsAngularVelocityInDegrees(MissionProfile->InitialAngularVelocityDegPerSec);
	}
}

void USuperHeavyAutopilotComponent::ResolveNavigationComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<USuperHeavyNavigationComponent*> NavigationComponents;
	Owner->GetComponents<USuperHeavyNavigationComponent>(NavigationComponents);
	for (USuperHeavyNavigationComponent* Component : NavigationComponents)
	{
		if (Component && (NavigationComponentName == NAME_None || Component->GetFName() == NavigationComponentName))
		{
			NavigationComponent = Component;
			return;
		}
	}
}

bool USuperHeavyAutopilotComponent::SetFlightPhase(ESuperHeavyFlightPhase NewPhase)
{
	if (!MissionProfile)
	{
		return false;
	}

	FSuperHeavyFlightPhaseConfig PhaseConfig;
	if (!MissionProfile->FindPhaseConfig(NewPhase, PhaseConfig))
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyAutopilotComponent: phase %d is not configured."), static_cast<int32>(NewPhase));
		return false;
	}

	ApplyPhaseConfig(PhaseConfig);
	return true;
}

void USuperHeavyAutopilotComponent::ApplyPhaseConfig(const FSuperHeavyFlightPhaseConfig& PhaseConfig)
{
	CurrentPhaseConfig = PhaseConfig;
	CurrentPhase = PhaseConfig.Phase;
	PhaseElapsedTimeSeconds = 0.0;
	ControlAccumulatorSeconds = 0.0;

	OuterEngines.bUseForThrottleControl = PhaseConfig.EngineGroupUsage.bOuterThrottleEnabled;
	OuterEngines.bUseForGimbalControl = false;
	InnerEngines.bUseForThrottleControl = PhaseConfig.EngineGroupUsage.bInnerThrottleEnabled;
	InnerEngines.bUseForGimbalControl = PhaseConfig.EngineGroupUsage.bInnerGimbalEnabled;
	CenterEngines.bUseForThrottleControl = PhaseConfig.EngineGroupUsage.bCenterThrottleEnabled;
	CenterEngines.bUseForGimbalControl = PhaseConfig.EngineGroupUsage.bCenterGimbalEnabled;

	ResetControllers();
}

void USuperHeavyAutopilotComponent::ResetControllers()
{
	CurrentPhaseConfig.Control.VerticalPositionPid.Reset();
	CurrentPhaseConfig.Control.VerticalVelocityPid.Reset();
	CurrentPhaseConfig.Control.LateralPositionXPid.Reset();
	CurrentPhaseConfig.Control.LateralPositionYPid.Reset();
	CurrentPhaseConfig.Control.LateralVelocityXPid.Reset();
	CurrentPhaseConfig.Control.LateralVelocityYPid.Reset();
	CurrentPhaseConfig.Control.AttitudePitchPid.Reset();
	CurrentPhaseConfig.Control.AttitudeRollPid.Reset();
}

void USuperHeavyAutopilotComponent::RunControlStep(double ControlDeltaTime)
{
	const FSuperHeavyMissionTarget MissionTarget = MissionProfile ? MissionProfile->Target : FSuperHeavyMissionTarget();
	const FSuperHeavyNavigationState State = NavigationComponent->CaptureNavigationState(MissionTarget.LandingTransform, MissionTarget.LandingTargetVelocityMps);

	PhaseElapsedTimeSeconds += ControlDeltaTime;
	EvaluatePhaseTransitions(State);

	LastDebugState = FSuperHeavyAutopilotDebugState();
	const FSuperHeavyActuatorCommand RawCommand = ComputeAutopilotCommand(State, ControlDeltaTime);
	LastCommand = SuperHeavyActuatorCommandUtils::Sanitize(RawCommand, OuterEngines, InnerEngines, CenterEngines, CurrentPhaseConfig.ActuatorLimits);
	LastDebugState.RawCommand = RawCommand;
	LastDebugState.Saturation = SuperHeavyActuatorCommandUtils::ComputeSaturation(RawCommand, LastCommand);
	LastDebugState.AvailableThrustN = GetAvailableThrottleThrustN();
	LastDebugState.EstimatedTWR = State.MassKg > UE_SMALL_NUMBER
		? EstimateCommandedThrustN(LastCommand) / (State.MassKg * GravityMps2)
		: 0.0;

	if (bApplyCommandsToVehicle)
	{
		ApplyCommand(LastCommand);
	}

	UpdateTelemetry(State);
}

void USuperHeavyAutopilotComponent::EvaluatePhaseTransitions(const FSuperHeavyNavigationState& State)
{
	if (!MissionProfile || AutopilotMode == ESuperHeavyAutopilotMode::Manual)
	{
		return;
	}

	for (const FSuperHeavyPhaseTransition& Transition : CurrentPhaseConfig.Transitions)
	{
		if (Transition.bEnabled && IsTransitionConditionMet(Transition, State))
		{
			SetFlightPhase(Transition.TargetPhase);
			return;
		}
	}
}

bool USuperHeavyAutopilotComponent::IsTransitionConditionMet(const FSuperHeavyPhaseTransition& Transition, const FSuperHeavyNavigationState& State) const
{
	switch (Transition.Condition)
	{
	case ESuperHeavyPhaseTransitionCondition::ElapsedTime:
		return PhaseElapsedTimeSeconds >= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::AltitudeAbove:
		return State.AltitudeM >= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::AltitudeBelow:
		return State.AltitudeM <= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::SpeedBelow:
		return State.VelocityWorldMps.Length() <= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::VerticalSpeedBelow:
		return FMath::Abs(State.VelocityWorldMps.Z) <= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::HorizontalDistanceBelow:
		return State.HorizontalDistanceToLandingTargetM <= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::DistanceToTargetBelow:
		return State.DistanceToLandingTargetM <= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::Touchdown:
		return State.AltitudeM <= Transition.Threshold && FMath::Abs(State.VelocityWorldMps.Z) <= Transition.SecondaryThreshold;
	default:
		return false;
	}
}

FSuperHeavyActuatorCommand USuperHeavyAutopilotComponent::ComputeAutopilotCommand(const FSuperHeavyNavigationState& State, double ControlDeltaTime)
{
	switch (CurrentPhaseConfig.GuidanceMode)
	{
	case ESuperHeavyGuidanceMode::VerticalAscent:
		return ComputeVerticalAscentCommand(State, ControlDeltaTime);
	case ESuperHeavyGuidanceMode::TargetApproach:
	case ESuperHeavyGuidanceMode::LandingBurn:
		return ComputeLandingCommand(State, ControlDeltaTime);
	default:
		return FSuperHeavyActuatorCommand();
	}
}

FSuperHeavyActuatorCommand USuperHeavyAutopilotComponent::ComputeVerticalAscentCommand(const FSuperHeavyNavigationState& State, double ControlDeltaTime)
{
	FSuperHeavyActuatorCommand Command;
	const double AvailableThrustN = GetAvailableThrottleThrustN();
	if (AvailableThrustN <= UE_SMALL_NUMBER || State.MassKg <= UE_SMALL_NUMBER)
	{
		return Command;
	}

	const double VelocityErrorMps = CurrentPhaseConfig.TargetVelocityWorldMps.Z - State.VelocityWorldMps.Z;
	const double DesiredVerticalAccelMps2 = CurrentPhaseConfig.Control.VerticalVelocityPid.Update(VelocityErrorMps, ControlDeltaTime);
	const double UpAlignment = FMath::Clamp(FVector::DotProduct(State.BodyUpWorld.GetSafeNormal(), FVector::UpVector), 0.2, 1.0);
	const double RequiredThrustN = State.MassKg * (GravityMps2 + DesiredVerticalAccelMps2) / UpAlignment;
	const double Throttle = RequiredThrustN / AvailableThrustN;

	LastDebugState.VelocityErrorMps = FVector(0.0, 0.0, VelocityErrorMps);
	LastDebugState.DesiredAccelerationWorldMps2 = FVector(0.0, 0.0, DesiredVerticalAccelMps2);
	LastDebugState.RequiredThrustN = RequiredThrustN;

	Command.bApplyOuterThrottle = OuterEngines.bUseForThrottleControl;
	Command.bApplyInnerThrottle = InnerEngines.bUseForThrottleControl;
	Command.bApplyCenterThrottle = CenterEngines.bUseForThrottleControl;
	Command.OuterThrottle = Throttle;
	Command.InnerThrottle = Throttle;
	Command.CenterThrottle = Throttle;

	ApplyAttitudeControl(State, CurrentPhaseConfig.TargetAttitudeWorldDeg, ControlDeltaTime, Command);
	return Command;
}

FSuperHeavyActuatorCommand USuperHeavyAutopilotComponent::ComputeLandingCommand(const FSuperHeavyNavigationState& State, double ControlDeltaTime)
{
	FSuperHeavyActuatorCommand Command;
	const double AvailableThrustN = GetAvailableThrottleThrustN();
	if (AvailableThrustN <= UE_SMALL_NUMBER || State.MassKg <= UE_SMALL_NUMBER)
	{
		return Command;
	}

	const FVector TargetPositionM = CurrentPhaseConfig.bUseMissionLandingTarget && MissionProfile
		? MissionProfile->Target.LandingTransform.GetLocation() / 100.0
		: FVector(State.LocationWorldM.X, State.LocationWorldM.Y, CurrentPhaseConfig.TargetAltitudeM);
	const FVector TargetVelocityMps = CurrentPhaseConfig.bUseMissionLandingTarget && MissionProfile
		? MissionProfile->Target.LandingTargetVelocityMps
		: CurrentPhaseConfig.TargetVelocityWorldMps;

	const FVector PositionErrorM = TargetPositionM - State.LocationWorldM;
	const FVector VelocityErrorMps = TargetVelocityMps - State.VelocityWorldMps;

	const double LateralAccelX = CurrentPhaseConfig.Control.LateralPositionXPid.Update(PositionErrorM.X, ControlDeltaTime)
		+ CurrentPhaseConfig.Control.LateralVelocityXPid.Update(VelocityErrorMps.X, ControlDeltaTime);
	const double LateralAccelY = CurrentPhaseConfig.Control.LateralPositionYPid.Update(PositionErrorM.Y, ControlDeltaTime)
		+ CurrentPhaseConfig.Control.LateralVelocityYPid.Update(VelocityErrorMps.Y, ControlDeltaTime);
	const double VerticalAccel = CurrentPhaseConfig.Control.VerticalPositionPid.Update(PositionErrorM.Z, ControlDeltaTime)
		+ CurrentPhaseConfig.Control.VerticalVelocityPid.Update(VelocityErrorMps.Z, ControlDeltaTime);

	const FVector DesiredAccelerationWorldMps2(LateralAccelX, LateralAccelY, VerticalAccel);
	const FVector DesiredForceWorldN = State.MassKg * (DesiredAccelerationWorldMps2 + FVector(0.0, 0.0, GravityMps2));
	const double RequiredThrustN = FMath::Max(0.0, FVector::DotProduct(DesiredForceWorldN, State.BodyUpWorld.GetSafeNormal()));
	const double Throttle = RequiredThrustN / AvailableThrustN;

	LastDebugState.PositionErrorM = PositionErrorM;
	LastDebugState.VelocityErrorMps = VelocityErrorMps;
	LastDebugState.DesiredAccelerationWorldMps2 = DesiredAccelerationWorldMps2;
	LastDebugState.RequiredThrustN = RequiredThrustN;

	Command.bApplyOuterThrottle = OuterEngines.bUseForThrottleControl;
	Command.bApplyInnerThrottle = InnerEngines.bUseForThrottleControl;
	Command.bApplyCenterThrottle = CenterEngines.bUseForThrottleControl;
	Command.OuterThrottle = Throttle;
	Command.InnerThrottle = Throttle;
	Command.CenterThrottle = Throttle;

	const FVector DesiredUpWorld = DesiredForceWorldN.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FQuat CurrentYawQuat = FRotator(0.0, CurrentPhaseConfig.TargetAttitudeWorldDeg.Yaw, 0.0).Quaternion();
	const FVector DesiredForwardProjected = FVector::VectorPlaneProject(CurrentYawQuat.GetForwardVector(), DesiredUpWorld).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FMatrix TargetMatrix = FRotationMatrix::MakeFromXZ(DesiredForwardProjected, DesiredUpWorld);
	ApplyAttitudeControl(State, TargetMatrix.Rotator(), ControlDeltaTime, Command);

	return Command;
}

void USuperHeavyAutopilotComponent::ApplyAttitudeControl(const FSuperHeavyNavigationState& State, const FRotator& TargetAttitudeWorldDeg, double ControlDeltaTime, FSuperHeavyActuatorCommand& Command)
{
	const FVector AttitudeErrorBodyDeg = SuperHeavyControlMath::ComputeAttitudeErrorBodyDeg(State.RotationWorldQuat, TargetAttitudeWorldDeg.Quaternion());
	const double PitchErrorDeg = SuperHeavyControlMath::GetBodyAxisValue(AttitudeErrorBodyDeg, PitchControlBodyAxis);
	const double RollErrorDeg = SuperHeavyControlMath::GetBodyAxisValue(AttitudeErrorBodyDeg, RollControlBodyAxis);
	const double PitchRateDegPerSec = SuperHeavyControlMath::GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, PitchControlBodyAxis);
	const double RollRateDegPerSec = SuperHeavyControlMath::GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, RollControlBodyAxis);

	const double PitchCommandDeg = CurrentPhaseConfig.Control.AttitudePitchPid.UpdateWithMeasuredRate(PitchErrorDeg, PitchRateDegPerSec, ControlDeltaTime) * GimbalPitchCommandSign;
	const double RollCommandDeg = CurrentPhaseConfig.Control.AttitudeRollPid.UpdateWithMeasuredRate(RollErrorDeg, RollRateDegPerSec, ControlDeltaTime) * GimbalRollCommandSign;

	LastDebugState.AttitudeErrorBodyDeg = AttitudeErrorBodyDeg;

	Command.bApplyInnerGimbal = InnerEngines.bUseForGimbalControl;
	Command.bApplyCenterGimbal = CenterEngines.bUseForGimbalControl;
	Command.InnerGimbalPitchDeg = PitchCommandDeg;
	Command.InnerGimbalRollDeg = RollCommandDeg;
	Command.CenterGimbalPitchDeg = PitchCommandDeg;
	Command.CenterGimbalRollDeg = RollCommandDeg;
}

void USuperHeavyAutopilotComponent::ApplyCommand(const FSuperHeavyActuatorCommand& Command)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetClass()->ImplementsInterface(USuperHeavyVehicleControlInterface::StaticClass()))
	{
		if (!bWarnedMissingVehicleControlInterface)
		{
			UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyAutopilotComponent: owner must implement SuperHeavyVehicleControlInterface."));
			bWarnedMissingVehicleControlInterface = true;
		}
		return;
	}

	ISuperHeavyVehicleControlInterface::Execute_ApplyActuatorCommand(Owner, Command);
}

void USuperHeavyAutopilotComponent::UpdateTelemetry(const FSuperHeavyNavigationState& State)
{
	LastTelemetry.AutopilotMode = AutopilotMode;
	LastTelemetry.FlightPhase = CurrentPhase;
	LastTelemetry.MissionId = MissionProfile ? MissionProfile->MissionId : NAME_None;
	LastTelemetry.PhaseElapsedTimeSeconds = PhaseElapsedTimeSeconds;
	LastTelemetry.Navigation = State;
	LastTelemetry.LastCommand = LastCommand;
	LastTelemetry.Debug = LastDebugState;
}

double USuperHeavyAutopilotComponent::GetAvailableThrottleThrustN() const
{
	return OuterEngines.GetMaxThrustN() + InnerEngines.GetMaxThrustN() + CenterEngines.GetMaxThrustN();
}

double USuperHeavyAutopilotComponent::EstimateCommandedThrustN(const FSuperHeavyActuatorCommand& Command) const
{
	return SuperHeavyControlMath::EstimateCommandedThrustN(OuterEngines, InnerEngines, CenterEngines, Command);
}

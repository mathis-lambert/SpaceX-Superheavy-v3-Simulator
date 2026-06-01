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

	if (NavigationComponent)
	{
		NavigationComponent->ResetNavigation();
	}

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
	OuterEngines.Engines.Reset();
	for (int32 Index = 1; Index <= 20; ++Index)
	{
		FSuperHeavyEngineDefinition Engine;
		Engine.EngineId = FName(*FString::Printf(TEXT("R%02d"), Index));
		Engine.AzimuthDeg = (Index - 1) * 18.0;
		OuterEngines.Engines.Add(Engine);
	}

	InnerEngines.GroupName = TEXT("Inner");
	InnerEngines.bUseForThrottleControl = true;
	InnerEngines.bUseForGimbalControl = true;
	InnerEngines.Engines.Reset();
	for (int32 Index = 1; Index <= 10; ++Index)
	{
		FSuperHeavyEngineDefinition Engine;
		Engine.EngineId = FName(*FString::Printf(TEXT("RGI%02d"), Index));
		Engine.AzimuthDeg = (Index - 1) * 36.0;
		InnerEngines.Engines.Add(Engine);
	}

	CenterEngines.GroupName = TEXT("Center");
	CenterEngines.bUseForThrottleControl = true;
	CenterEngines.bUseForGimbalControl = true;
	CenterEngines.Engines = {
		{ TEXT("RGC01"), 90.0 },
		{ TEXT("RGC02"), 210.0 },
		{ TEXT("RGC03"), 330.0 }
	};
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
	ApplyPhaseActuatorHandoff(PhaseConfig);

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

void USuperHeavyAutopilotComponent::ApplyPhaseActuatorHandoff(const FSuperHeavyFlightPhaseConfig& NextPhaseConfig)
{
	if (!bApplyCommandsToVehicle)
	{
		return;
	}

	FSuperHeavyActuatorCommand HandoffCommand;
	if (OuterEngines.bUseForThrottleControl && !NextPhaseConfig.EngineGroupUsage.bOuterThrottleEnabled)
	{
		AppendThrottleCommands(HandoffCommand, OuterEngines, 0.0);
	}
	if (InnerEngines.bUseForThrottleControl && !NextPhaseConfig.EngineGroupUsage.bInnerThrottleEnabled)
	{
		AppendThrottleCommands(HandoffCommand, InnerEngines, 0.0);
	}
	if (CenterEngines.bUseForThrottleControl && !NextPhaseConfig.EngineGroupUsage.bCenterThrottleEnabled)
	{
		AppendThrottleCommands(HandoffCommand, CenterEngines, 0.0);
	}
	if (InnerEngines.bUseForGimbalControl && !NextPhaseConfig.EngineGroupUsage.bInnerGimbalEnabled)
	{
		AppendGimbalCommands(HandoffCommand, InnerEngines, 0.0, 0.0, 0.0);
	}
	if (CenterEngines.bUseForGimbalControl && !NextPhaseConfig.EngineGroupUsage.bCenterGimbalEnabled)
	{
		AppendGimbalCommands(HandoffCommand, CenterEngines, 0.0, 0.0, 0.0);
	}

	if (HandoffCommand.EngineCommands.IsEmpty())
	{
		return;
	}

	FSuperHeavyActuatorLimits HandoffLimits = NextPhaseConfig.ActuatorLimits;
	HandoffLimits.MinThrottle = 0.0;

	const FSuperHeavyActuatorCommand SanitizedHandoff = SuperHeavyActuatorCommandUtils::Sanitize(
		HandoffCommand,
		OuterEngines,
		InnerEngines,
		CenterEngines,
		HandoffLimits);

	ApplyCommand(SanitizedHandoff);
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
	CurrentPhaseConfig.Control.AttitudeYawPid.Reset();
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
	case ESuperHeavyPhaseTransitionCondition::VerticalVelocityBelow:
		return State.VelocityWorldMps.Z <= Transition.Threshold;
	case ESuperHeavyPhaseTransitionCondition::VerticalVelocityAbove:
		return State.VelocityWorldMps.Z >= Transition.Threshold;
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
	FVector LateralAccelerationWorldMps2 = FVector::ZeroVector;

	if (CurrentPhaseConfig.bUseMissionLaunchPositionXY && MissionProfile)
	{
		const FVector LaunchPositionM = MissionProfile->Target.LaunchTransform.GetLocation() / 100.0;
		const FVector PositionErrorM(LaunchPositionM.X - State.LocationWorldM.X, LaunchPositionM.Y - State.LocationWorldM.Y, 0.0);
		const FVector VelocityErrorWorldMps(-State.VelocityWorldMps.X, -State.VelocityWorldMps.Y, 0.0);
		LateralAccelerationWorldMps2.X =
			CurrentPhaseConfig.Control.LateralPositionXPid.Update(PositionErrorM.X, ControlDeltaTime)
			+ CurrentPhaseConfig.Control.LateralVelocityXPid.Update(VelocityErrorWorldMps.X, ControlDeltaTime);
		LateralAccelerationWorldMps2.Y =
			CurrentPhaseConfig.Control.LateralPositionYPid.Update(PositionErrorM.Y, ControlDeltaTime)
			+ CurrentPhaseConfig.Control.LateralVelocityYPid.Update(VelocityErrorWorldMps.Y, ControlDeltaTime);

		const double MaxLateralAccelerationMps2 = FMath::Max(0.0, CurrentPhaseConfig.Control.MaxLateralAccelerationMps2);
		if (MaxLateralAccelerationMps2 > UE_SMALL_NUMBER)
		{
			LateralAccelerationWorldMps2 = LateralAccelerationWorldMps2.GetClampedToMaxSize(MaxLateralAccelerationMps2);
		}
	}

	const FVector DesiredSpecificForceWorldMps2 =
		LateralAccelerationWorldMps2 + FVector(0.0, 0.0, GravityMps2 + DesiredVerticalAccelMps2);
	const double RequiredThrustN = State.MassKg * DesiredSpecificForceWorldMps2.Length();
	const double Throttle = RequiredThrustN / AvailableThrustN;

	LastDebugState.VelocityErrorMps = FVector(-State.VelocityWorldMps.X, -State.VelocityWorldMps.Y, VelocityErrorMps);
	LastDebugState.DesiredAccelerationWorldMps2 = FVector(LateralAccelerationWorldMps2.X, LateralAccelerationWorldMps2.Y, DesiredVerticalAccelMps2);
	LastDebugState.RequiredThrustN = RequiredThrustN;

	AppendThrottleCommands(Command, OuterEngines, Throttle);
	AppendThrottleCommands(Command, InnerEngines, Throttle);
	AppendThrottleCommands(Command, CenterEngines, Throttle);

	const FVector DesiredUpWorld = DesiredSpecificForceWorldMps2.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FQuat CurrentYawQuat = FRotator(0.0, CurrentPhaseConfig.TargetAttitudeWorldDeg.Yaw, 0.0).Quaternion();
	const FVector DesiredForwardProjected = FVector::VectorPlaneProject(CurrentYawQuat.GetForwardVector(), DesiredUpWorld).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FMatrix TargetMatrix = FRotationMatrix::MakeFromXZ(DesiredForwardProjected, DesiredUpWorld);
	ApplyAttitudeControl(State, TargetMatrix.Rotator(), ControlDeltaTime, Command);
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
	const double TargetAltitudeM = CurrentPhaseConfig.bUseMissionLandingTarget && MissionProfile && NavigationComponent
		? (MissionProfile->Target.LandingTransform.GetLocation().Z - NavigationComponent->AltitudeReferenceWorldZCm) / 100.0
		: CurrentPhaseConfig.TargetAltitudeM;
	const FVector TargetVelocityMps = CurrentPhaseConfig.bUseMissionLandingTarget && MissionProfile
		? MissionProfile->Target.LandingTargetVelocityMps
		: CurrentPhaseConfig.TargetVelocityWorldMps;

	FVector PositionErrorM = TargetPositionM - State.LocationWorldM;
	PositionErrorM.Z = TargetAltitudeM - State.AltitudeM;
	const FVector VelocityErrorMps = TargetVelocityMps - State.VelocityWorldMps;

	const double LateralAccelX = CurrentPhaseConfig.Control.LateralPositionXPid.Update(PositionErrorM.X, ControlDeltaTime)
		+ CurrentPhaseConfig.Control.LateralVelocityXPid.Update(VelocityErrorMps.X, ControlDeltaTime);
	const double LateralAccelY = CurrentPhaseConfig.Control.LateralPositionYPid.Update(PositionErrorM.Y, ControlDeltaTime)
		+ CurrentPhaseConfig.Control.LateralVelocityYPid.Update(VelocityErrorMps.Y, ControlDeltaTime);
	double VerticalAccel = CurrentPhaseConfig.Control.VerticalPositionPid.Update(PositionErrorM.Z, ControlDeltaTime)
		+ CurrentPhaseConfig.Control.VerticalVelocityPid.Update(VelocityErrorMps.Z, ControlDeltaTime);
	const double HeightAboveTargetM = FMath::Max(State.AltitudeM - TargetAltitudeM, CurrentPhaseConfig.Control.MinVerticalBrakingDistanceM);
	const double DescentSpeedMps = FMath::Max(0.0, -State.VelocityWorldMps.Z);
	const double TargetDescentSpeedMps = FMath::Max(0.0, -TargetVelocityMps.Z);
	const double RequiredBrakingAccelerationMps2 = FMath::Max(
		0.0,
		((DescentSpeedMps * DescentSpeedMps) - (TargetDescentSpeedMps * TargetDescentSpeedMps)) / (2.0 * HeightAboveTargetM))
		* FMath::Max(1.0, CurrentPhaseConfig.Control.VerticalBrakingSafetyFactor);
	VerticalAccel = FMath::Max(VerticalAccel, RequiredBrakingAccelerationMps2);

	FVector LateralAccelerationWorldMps2(LateralAccelX, LateralAccelY, 0.0);
	const double MaxLateralAccelerationMps2 = FMath::Max(0.0, CurrentPhaseConfig.Control.MaxLateralAccelerationMps2);
	if (MaxLateralAccelerationMps2 > UE_SMALL_NUMBER)
	{
		LateralAccelerationWorldMps2 = LateralAccelerationWorldMps2.GetClampedToMaxSize(MaxLateralAccelerationMps2);
	}

	const double MaxVerticalAccelerationMps2 = FMath::Max(0.0, CurrentPhaseConfig.Control.MaxVerticalAccelerationMps2);
	const double ShapedVerticalAccelerationMps2 = MaxVerticalAccelerationMps2 > UE_SMALL_NUMBER
		? FMath::Clamp(VerticalAccel, -MaxVerticalAccelerationMps2, MaxVerticalAccelerationMps2)
		: VerticalAccel;

	const double UpwardSpecificForceMps2 = FMath::Max(0.0, GravityMps2 + ShapedVerticalAccelerationMps2);
	const double MaxTiltRad = FMath::DegreesToRadians(FMath::Clamp(CurrentPhaseConfig.Control.MaxTargetTiltDeg, 0.0, 85.0));
	const double MaxTiltLimitedLateralAccelerationMps2 = UpwardSpecificForceMps2 * FMath::Tan(MaxTiltRad);
	LateralAccelerationWorldMps2 = LateralAccelerationWorldMps2.GetClampedToMaxSize(MaxTiltLimitedLateralAccelerationMps2);

	const FVector DesiredAccelerationWorldMps2(
		LateralAccelerationWorldMps2.X,
		LateralAccelerationWorldMps2.Y,
		ShapedVerticalAccelerationMps2);
	const FVector DesiredSpecificForceWorldMps2 = DesiredAccelerationWorldMps2 + FVector(0.0, 0.0, GravityMps2);
	const double RequiredThrustN = State.MassKg * DesiredSpecificForceWorldMps2.Length();
	const double Throttle = RequiredThrustN / AvailableThrustN;

	LastDebugState.PositionErrorM = PositionErrorM;
	LastDebugState.VelocityErrorMps = VelocityErrorMps;
	LastDebugState.DesiredAccelerationWorldMps2 = DesiredAccelerationWorldMps2;
	LastDebugState.RequiredThrustN = RequiredThrustN;

	AppendThrottleCommands(Command, OuterEngines, Throttle);
	AppendThrottleCommands(Command, InnerEngines, Throttle);
	AppendThrottleCommands(Command, CenterEngines, Throttle);

	const FVector DesiredUpWorld = DesiredSpecificForceWorldMps2.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
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
	const double YawErrorDeg = SuperHeavyControlMath::GetBodyAxisValue(AttitudeErrorBodyDeg, ESuperHeavyBodyAxis::Z);
	const double PitchRateDegPerSec = SuperHeavyControlMath::GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, PitchControlBodyAxis);
	const double RollRateDegPerSec = SuperHeavyControlMath::GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, RollControlBodyAxis);
	const double YawRateDegPerSec = SuperHeavyControlMath::GetBodyAxisValue(State.AngularVelocityBodyDegPerSec, ESuperHeavyBodyAxis::Z);

	const double PitchCommandDeg = CurrentPhaseConfig.Control.AttitudePitchPid.UpdateWithMeasuredRate(PitchErrorDeg, PitchRateDegPerSec, ControlDeltaTime) * GimbalPitchCommandSign;
	const double RollCommandDeg = CurrentPhaseConfig.Control.AttitudeRollPid.UpdateWithMeasuredRate(RollErrorDeg, RollRateDegPerSec, ControlDeltaTime) * GimbalRollCommandSign;
	const double YawCommandDeg = FMath::Clamp(
		CurrentPhaseConfig.Control.AttitudeYawPid.UpdateWithMeasuredRate(YawErrorDeg, YawRateDegPerSec, ControlDeltaTime) * GimbalYawCommandSign,
		-CurrentPhaseConfig.Control.MaxYawGimbalMixDeg,
		CurrentPhaseConfig.Control.MaxYawGimbalMixDeg);

	LastDebugState.AttitudeErrorBodyDeg = AttitudeErrorBodyDeg;

	AppendGimbalCommands(Command, InnerEngines, PitchCommandDeg, RollCommandDeg, YawCommandDeg);
	AppendGimbalCommands(Command, CenterEngines, PitchCommandDeg, RollCommandDeg, YawCommandDeg);
}

void USuperHeavyAutopilotComponent::AppendThrottleCommands(FSuperHeavyActuatorCommand& Command, const FSuperHeavyEngineGroupConfig& Group, double Throttle) const
{
	if (!Group.bUseForThrottleControl)
	{
		return;
	}

	for (const FSuperHeavyEngineDefinition& Engine : Group.Engines)
	{
		FSuperHeavyEngineActuatorCommand& EngineCommand = Command.EngineCommands.AddDefaulted_GetRef();
		EngineCommand.EngineId = Engine.EngineId;
		EngineCommand.bApplyThrottle = true;
		EngineCommand.Throttle = Throttle;
	}
}

void USuperHeavyAutopilotComponent::AppendGimbalCommands(FSuperHeavyActuatorCommand& Command, const FSuperHeavyEngineGroupConfig& Group, double PitchDeg, double RollDeg, double YawDeg) const
{
	if (!Group.bUseForGimbalControl)
	{
		return;
	}

	for (const FSuperHeavyEngineDefinition& Engine : Group.Engines)
	{
		const double AzimuthRad = FMath::DegreesToRadians(Engine.AzimuthDeg);
		const FVector2D TangentialDirection(-FMath::Sin(AzimuthRad), FMath::Cos(AzimuthRad));
		const double MixedPitchDeg = PitchDeg + YawDeg * TangentialDirection.X;
		const double MixedRollDeg = RollDeg + YawDeg * TangentialDirection.Y;

		FSuperHeavyEngineActuatorCommand* EngineCommand = Command.EngineCommands.FindByPredicate([&Engine](const FSuperHeavyEngineActuatorCommand& ExistingCommand)
		{
			return ExistingCommand.EngineId == Engine.EngineId;
		});
		if (!EngineCommand)
		{
			EngineCommand = &Command.EngineCommands.AddDefaulted_GetRef();
			EngineCommand->EngineId = Engine.EngineId;
		}

		EngineCommand->bApplyGimbal = true;
		EngineCommand->GimbalPitchDeg = MixedPitchDeg;
		EngineCommand->GimbalRollDeg = MixedRollDeg;
	}
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

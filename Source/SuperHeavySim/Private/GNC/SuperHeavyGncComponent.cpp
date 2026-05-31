#include "GNC/SuperHeavyGncComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

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

	AttitudePitchPid.Kp = 0.25;
	AttitudePitchPid.Kd = 0.05;
	AttitudePitchPid.bClampOutput = true;
	AttitudePitchPid.OutputMin = -ActuatorLimits.MaxGimbalDeg;
	AttitudePitchPid.OutputMax = ActuatorLimits.MaxGimbalDeg;

	AttitudeRollPid = AttitudePitchPid;
}

void USuperHeavyGncComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolvePhysicsComponent();
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
	LastCommand = ComputeCommand(LastState, ControlDeltaTime);

	if (bApplyCommandsToBlueprint)
	{
		ApplyCommand(LastCommand);
	}
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
	State.RotationWorldDeg = Body->GetComponentRotation();
	State.BodyUpWorld = Body->GetUpVector();
	State.AltitudeM = (State.LocationWorldCm.Z - AltitudeReferenceWorldZCm) / 100.0;
	State.VerticalSpeedMps = State.VelocityWorldMps.Z;
	State.MassKg = Body->GetMass();

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
	const double PitchErrorDeg = FMath::FindDeltaAngleDegrees(State.RotationWorldDeg.Pitch, Targets.TargetWorldAttitudeDeg.Pitch);
	const double RollErrorDeg = FMath::FindDeltaAngleDegrees(State.RotationWorldDeg.Roll, Targets.TargetWorldAttitudeDeg.Roll);

	const double PitchCommandDeg = FMath::Clamp(
		AttitudePitchPid.Update(PitchErrorDeg, ControlDeltaTime) * GimbalPitchCommandSign,
		-ActuatorLimits.MaxGimbalDeg,
		ActuatorLimits.MaxGimbalDeg);

	const double RollCommandDeg = FMath::Clamp(
		AttitudeRollPid.Update(RollErrorDeg, ControlDeltaTime) * GimbalRollCommandSign,
		-ActuatorLimits.MaxGimbalDeg,
		ActuatorLimits.MaxGimbalDeg);

	Command.CenterGimbalPitchDeg = CenterEngines.bUseForGimbalControl ? PitchCommandDeg : 0.0;
	Command.CenterGimbalRollDeg = CenterEngines.bUseForGimbalControl ? RollCommandDeg : 0.0;
	Command.InnerGimbalPitchDeg = InnerEngines.bUseForGimbalControl ? PitchCommandDeg : 0.0;
	Command.InnerGimbalRollDeg = InnerEngines.bUseForGimbalControl ? RollCommandDeg : 0.0;
}

void USuperHeavyGncComponent::ApplyCommand(const FSuperHeavyActuatorCommand& Command)
{
	SetGroupThrottle(OuterEngines, Command.OuterThrottle);
	SetGroupThrottle(InnerEngines, Command.InnerThrottle);
	SetGroupThrottle(CenterEngines, Command.CenterThrottle);

	SetGroupGimbal(InnerEngines, Command.InnerGimbalPitchDeg, Command.InnerGimbalRollDeg);
	SetGroupGimbal(CenterEngines, Command.CenterGimbalPitchDeg, Command.CenterGimbalRollDeg);

	InvokeNameFloatFunction(SetGridFinAngleFunctionName, TEXT("GF_XP"), FMath::Clamp(Command.GridFinXPCommandDeg, -ActuatorLimits.MaxGridFinDeg, ActuatorLimits.MaxGridFinDeg));
	InvokeNameFloatFunction(SetGridFinAngleFunctionName, TEXT("GF_XM"), FMath::Clamp(Command.GridFinXMCommandDeg, -ActuatorLimits.MaxGridFinDeg, ActuatorLimits.MaxGridFinDeg));
	InvokeNameFloatFunction(SetGridFinAngleFunctionName, TEXT("GF_YM"), FMath::Clamp(Command.GridFinYMCommandDeg, -ActuatorLimits.MaxGridFinDeg, ActuatorLimits.MaxGridFinDeg));
}

double USuperHeavyGncComponent::GetAvailableThrottleThrustN() const
{
	return OuterEngines.GetMaxThrustN() + InnerEngines.GetMaxThrustN() + CenterEngines.GetMaxThrustN();
}

void USuperHeavyGncComponent::SetGroupThrottle(const FSuperHeavyEngineGroupConfig& Group, double Throttle)
{
	const double ClampedThrottle = FMath::Clamp(Throttle, ActuatorLimits.MinThrottle, ActuatorLimits.MaxThrottle);
	for (const FName EngineId : Group.EngineIds)
	{
		InvokeNameFloatFunction(SetEngineThrottleFunctionName, EngineId, ClampedThrottle);
	}
}

void USuperHeavyGncComponent::SetGroupGimbal(const FSuperHeavyEngineGroupConfig& Group, double PitchDeg, double RollDeg)
{
	if (!Group.bUseForGimbalControl)
	{
		return;
	}

	const double ClampedPitchDeg = FMath::Clamp(PitchDeg, -ActuatorLimits.MaxGimbalDeg, ActuatorLimits.MaxGimbalDeg);
	const double ClampedRollDeg = FMath::Clamp(RollDeg, -ActuatorLimits.MaxGimbalDeg, ActuatorLimits.MaxGimbalDeg);
	for (const FName EngineId : Group.EngineIds)
	{
		InvokeNameTwoFloatFunction(SetEngineGimbalFunctionName, EngineId, ClampedPitchDeg, ClampedRollDeg);
	}
}

bool USuperHeavyGncComponent::InvokeNameFloatFunction(FName FunctionName, FName Id, double Value) const
{
	AActor* Owner = GetOwner();
	UFunction* Function = Owner ? Owner->FindFunction(FunctionName) : nullptr;
	if (!Owner || !Function)
	{
		return false;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize));
	FMemory::Memzero(Params, Function->ParmsSize);
	SetInputParamsByType(Function, Params, &Id, &Value, nullptr);
	Owner->ProcessEvent(Function, Params);
	return true;
}

bool USuperHeavyGncComponent::InvokeNameTwoFloatFunction(FName FunctionName, FName Id, double FirstValue, double SecondValue) const
{
	AActor* Owner = GetOwner();
	UFunction* Function = Owner ? Owner->FindFunction(FunctionName) : nullptr;
	if (!Owner || !Function)
	{
		return false;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize));
	FMemory::Memzero(Params, Function->ParmsSize);
	SetInputParamsByType(Function, Params, &Id, &FirstValue, &SecondValue);
	Owner->ProcessEvent(Function, Params);
	return true;
}

void USuperHeavyGncComponent::SetInputParamsByType(UFunction* Function, void* Params, const FName* NameValue, const double* FirstValue, const double* SecondValue)
{
	int32 FloatParamIndex = 0;
	bool bNameSet = false;

	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			if (NameValue && !bNameSet)
			{
				NameProperty->SetPropertyValue_InContainer(Params, *NameValue);
				bNameSet = true;
			}
			continue;
		}

		const double* FloatValue = FloatParamIndex == 0 ? FirstValue : SecondValue;
		if (!FloatValue)
		{
			continue;
		}

		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
		{
			FloatProperty->SetPropertyValue_InContainer(Params, static_cast<float>(*FloatValue));
			++FloatParamIndex;
		}
		else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			DoubleProperty->SetPropertyValue_InContainer(Params, *FloatValue);
			++FloatParamIndex;
		}
	}
}

#include "GNC/SuperHeavyFlightPhaseProfile.h"

namespace
{
FString PhaseToString(ESuperHeavyFlightPhase Phase)
{
	const UEnum* Enum = StaticEnum<ESuperHeavyFlightPhase>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Phase)).ToString() : FString::FromInt(static_cast<int32>(Phase));
}

void AddError(FSuperHeavyFlightPhaseValidationResult& Result, FString&& Message)
{
	Result.bIsValid = false;
	Result.Errors.Add(MoveTemp(Message));
}

bool HasThrottleGroup(const FSuperHeavyEngineGroupUsage& Usage)
{
	return Usage.bOuterThrottleEnabled || Usage.bInnerThrottleEnabled || Usage.bCenterThrottleEnabled;
}

bool HasGimbalGroup(const FSuperHeavyEngineGroupUsage& Usage)
{
	return Usage.bInnerGimbalEnabled || Usage.bCenterGimbalEnabled;
}

FString TransitionConditionToString(ESuperHeavyPhaseTransitionCondition Condition)
{
	const UEnum* Enum = StaticEnum<ESuperHeavyPhaseTransitionCondition>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Condition)).ToString() : FString::FromInt(static_cast<int32>(Condition));
}
}

bool USuperHeavyFlightPhaseProfile::FindConfigForPhase(ESuperHeavyFlightPhase Phase, FSuperHeavyFlightPhaseConfig& OutConfig) const
{
	for (const FSuperHeavyFlightPhaseConfig& Config : Phases)
	{
		if (Config.Phase == Phase)
		{
			OutConfig = Config;
			return true;
		}
	}

	return false;
}

FSuperHeavyFlightPhaseValidationResult USuperHeavyFlightPhaseProfile::ValidateProfile() const
{
	FSuperHeavyFlightPhaseValidationResult Result;

	if (Phases.IsEmpty())
	{
		AddError(Result, TEXT("Flight phase profile has no phase configs."));
		return Result;
	}

	TSet<ESuperHeavyFlightPhase> SeenPhases;
	for (const FSuperHeavyFlightPhaseConfig& Config : Phases)
	{
		const FString PhaseName = PhaseToString(Config.Phase);

		if (SeenPhases.Contains(Config.Phase))
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' is configured more than once."), *PhaseName));
		}
		SeenPhases.Add(Config.Phase);

		if (Config.ControlRateHz < 1.0 || Config.ControlRateHz > 500.0)
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has ControlRateHz outside [1, 500]."), *PhaseName));
		}

		if (Config.ActuatorLimits.MinThrottle < 0.0 || Config.ActuatorLimits.MaxThrottle > 1.0 || Config.ActuatorLimits.MinThrottle > Config.ActuatorLimits.MaxThrottle)
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has invalid throttle limits."), *PhaseName));
		}

		if (Config.ActuatorLimits.MaxGimbalDeg < 0.0)
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has a negative max gimbal angle."), *PhaseName));
		}

		if (Config.ActuatorLimits.MaxGridFinDeg < 0.0)
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has a negative max grid fin angle."), *PhaseName));
		}

		if (Config.bEnableGnc && Config.GuidanceMode == ESuperHeavyGuidanceMode::Disabled)
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' enables GNC but uses Disabled guidance mode."), *PhaseName));
		}

		if (Config.bEnableGnc && Config.GuidanceMode != ESuperHeavyGuidanceMode::Disabled && !HasThrottleGroup(Config.EngineGroupUsage))
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has automatic guidance but no throttle group enabled."), *PhaseName));
		}

		if (Config.bEnableAttitudeHold && !HasGimbalGroup(Config.EngineGroupUsage))
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' enables attitude hold but no gimbaled engine group is enabled."), *PhaseName));
		}

		if (!Config.bEnableGnc && Config.GuidanceMode != ESuperHeavyGuidanceMode::Disabled)
		{
			Result.Warnings.Add(FString::Printf(TEXT("Phase '%s' has a guidance mode set but GNC is disabled."), *PhaseName));
		}

		for (int32 TransitionIndex = 0; TransitionIndex < Config.Transitions.Num(); ++TransitionIndex)
		{
			const FSuperHeavyFlightPhaseTransition& Transition = Config.Transitions[TransitionIndex];
			if (!Transition.bEnabled)
			{
				continue;
			}

			const FString TargetPhaseName = PhaseToString(Transition.TargetPhase);
			const FString ConditionName = TransitionConditionToString(Transition.Condition);

			if (Transition.TargetPhase == Config.Phase)
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d targets itself."), *PhaseName, TransitionIndex));
			}

			if (Transition.TargetPhase != ESuperHeavyFlightPhase::Manual && !IsPhaseConfigured(Transition.TargetPhase))
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d targets unconfigured phase '%s'."), *PhaseName, TransitionIndex, *TargetPhaseName));
			}

			if (Transition.Condition == ESuperHeavyPhaseTransitionCondition::ElapsedTime && Transition.Threshold < 0.0)
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d has negative elapsed time threshold."), *PhaseName, TransitionIndex));
			}

			if (Transition.Condition == ESuperHeavyPhaseTransitionCondition::Touchdown && Transition.SecondaryThreshold < 0.0)
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d has negative touchdown vertical-speed threshold."), *PhaseName, TransitionIndex));
			}

			if (Transition.TargetPhase == ESuperHeavyFlightPhase::Manual)
			{
				Result.Warnings.Add(FString::Printf(TEXT("Phase '%s' transition %d uses condition '%s' to enter Manual. This is valid but stops automatic guidance."), *PhaseName, TransitionIndex, *ConditionName));
			}
		}
	}

	if (!SeenPhases.Contains(ESuperHeavyFlightPhase::Manual))
	{
		Result.Warnings.Add(TEXT("Manual phase is not configured. This is valid because Manual is handled by the GNC component, but add it if you want profile documentation."));
	}

	return Result;
}

bool USuperHeavyFlightPhaseProfile::IsPhaseConfigured(ESuperHeavyFlightPhase Phase) const
{
	for (const FSuperHeavyFlightPhaseConfig& Config : Phases)
	{
		if (Config.Phase == Phase)
		{
			return true;
		}
	}

	return false;
}

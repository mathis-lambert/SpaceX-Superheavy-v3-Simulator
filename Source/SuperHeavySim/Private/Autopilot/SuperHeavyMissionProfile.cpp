#include "Autopilot/SuperHeavyMissionProfile.h"

namespace
{
void AddError(FSuperHeavyMissionValidationResult& Result, FString&& Error)
{
	Result.bIsValid = false;
	Result.Errors.Add(MoveTemp(Error));
}

FString PhaseToString(ESuperHeavyFlightPhase Phase)
{
	const UEnum* Enum = StaticEnum<ESuperHeavyFlightPhase>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Phase)).ToString() : FString::FromInt(static_cast<int32>(Phase));
}

bool HasThrottleGroup(const FSuperHeavyEngineGroupUsage& Usage)
{
	return Usage.bOuterThrottleEnabled || Usage.bInnerThrottleEnabled || Usage.bCenterThrottleEnabled;
}

bool HasGimbalGroup(const FSuperHeavyEngineGroupUsage& Usage)
{
	return Usage.bInnerGimbalEnabled || Usage.bCenterGimbalEnabled;
}
}

bool USuperHeavyMissionProfile::FindPhaseConfig(ESuperHeavyFlightPhase Phase, FSuperHeavyFlightPhaseConfig& OutConfig) const
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

bool USuperHeavyMissionProfile::IsPhaseConfigured(ESuperHeavyFlightPhase Phase) const
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

FSuperHeavyMissionValidationResult USuperHeavyMissionProfile::ValidateMission() const
{
	FSuperHeavyMissionValidationResult Result;

	if (Phases.IsEmpty())
	{
		AddError(Result, TEXT("Mission has no flight phases."));
		return Result;
	}

	if (!IsPhaseConfigured(InitialPhase))
	{
		AddError(Result, FString::Printf(TEXT("Initial phase '%s' is not configured."), *PhaseToString(InitialPhase)));
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

		if (Config.GuidanceMode != ESuperHeavyGuidanceMode::Disabled && !HasThrottleGroup(Config.EngineGroupUsage))
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has guidance enabled but no throttle group enabled."), *PhaseName));
		}

		if ((Config.GuidanceMode == ESuperHeavyGuidanceMode::LandingBurn || Config.GuidanceMode == ESuperHeavyGuidanceMode::TargetApproach) && !HasGimbalGroup(Config.EngineGroupUsage))
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' needs gimbal authority but no gimbal group is enabled."), *PhaseName));
		}

		if (Config.ActuatorLimits.MinThrottle < 0.0 || Config.ActuatorLimits.MaxThrottle > 1.0 || Config.ActuatorLimits.MinThrottle > Config.ActuatorLimits.MaxThrottle)
		{
			AddError(Result, FString::Printf(TEXT("Phase '%s' has invalid throttle limits."), *PhaseName));
		}

		for (int32 TransitionIndex = 0; TransitionIndex < Config.Transitions.Num(); ++TransitionIndex)
		{
			const FSuperHeavyPhaseTransition& Transition = Config.Transitions[TransitionIndex];
			if (!Transition.bEnabled)
			{
				continue;
			}

			if (Transition.TargetPhase == Config.Phase)
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d targets itself."), *PhaseName, TransitionIndex));
			}

			if (!IsPhaseConfigured(Transition.TargetPhase))
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d targets unconfigured phase '%s'."), *PhaseName, TransitionIndex, *PhaseToString(Transition.TargetPhase)));
			}

			if (Transition.Threshold < 0.0)
			{
				AddError(Result, FString::Printf(TEXT("Phase '%s' transition %d has a negative threshold."), *PhaseName, TransitionIndex));
			}
		}
	}

	return Result;
}

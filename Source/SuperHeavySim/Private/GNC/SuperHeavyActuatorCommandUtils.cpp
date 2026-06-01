#include "GNC/SuperHeavyActuatorCommandUtils.h"

namespace
{
double ClampIfApplied(bool bApply, double Value, double MinValue, double MaxValue)
{
	return bApply ? FMath::Clamp(Value, MinValue, MaxValue) : 0.0;
}

bool Differs(double A, double B)
{
	constexpr double Tolerance = 1.0e-6;
	return !FMath::IsNearlyEqual(A, B, Tolerance);
}
}

namespace SuperHeavyActuatorCommandUtils
{
FSuperHeavyActuatorCommand Sanitize(
	const FSuperHeavyActuatorCommand& Command,
	const FSuperHeavyEngineGroupConfig& OuterEngines,
	const FSuperHeavyEngineGroupConfig& InnerEngines,
	const FSuperHeavyEngineGroupConfig& CenterEngines,
	const FSuperHeavyActuatorLimits& Limits)
{
	FSuperHeavyActuatorCommand Sanitized = Command;

	Sanitized.bApplyOuterThrottle = OuterEngines.bUseForThrottleControl && Command.bApplyOuterThrottle;
	Sanitized.bApplyInnerThrottle = InnerEngines.bUseForThrottleControl && Command.bApplyInnerThrottle;
	Sanitized.bApplyCenterThrottle = CenterEngines.bUseForThrottleControl && Command.bApplyCenterThrottle;
	Sanitized.bApplyInnerGimbal = InnerEngines.bUseForGimbalControl && Command.bApplyInnerGimbal;
	Sanitized.bApplyCenterGimbal = CenterEngines.bUseForGimbalControl && Command.bApplyCenterGimbal;

	Sanitized.OuterThrottle = ClampIfApplied(Sanitized.bApplyOuterThrottle, Sanitized.OuterThrottle, Limits.MinThrottle, Limits.MaxThrottle);
	Sanitized.InnerThrottle = ClampIfApplied(Sanitized.bApplyInnerThrottle, Sanitized.InnerThrottle, Limits.MinThrottle, Limits.MaxThrottle);
	Sanitized.CenterThrottle = ClampIfApplied(Sanitized.bApplyCenterThrottle, Sanitized.CenterThrottle, Limits.MinThrottle, Limits.MaxThrottle);

	Sanitized.InnerGimbalPitchDeg = ClampIfApplied(Sanitized.bApplyInnerGimbal, Sanitized.InnerGimbalPitchDeg, -Limits.MaxGimbalDeg, Limits.MaxGimbalDeg);
	Sanitized.InnerGimbalRollDeg = ClampIfApplied(Sanitized.bApplyInnerGimbal, Sanitized.InnerGimbalRollDeg, -Limits.MaxGimbalDeg, Limits.MaxGimbalDeg);
	Sanitized.CenterGimbalPitchDeg = ClampIfApplied(Sanitized.bApplyCenterGimbal, Sanitized.CenterGimbalPitchDeg, -Limits.MaxGimbalDeg, Limits.MaxGimbalDeg);
	Sanitized.CenterGimbalRollDeg = ClampIfApplied(Sanitized.bApplyCenterGimbal, Sanitized.CenterGimbalRollDeg, -Limits.MaxGimbalDeg, Limits.MaxGimbalDeg);

	Sanitized.GridFinXPCommandDeg = FMath::Clamp(Sanitized.GridFinXPCommandDeg, -Limits.MaxGridFinDeg, Limits.MaxGridFinDeg);
	Sanitized.GridFinXMCommandDeg = FMath::Clamp(Sanitized.GridFinXMCommandDeg, -Limits.MaxGridFinDeg, Limits.MaxGridFinDeg);
	Sanitized.GridFinYMCommandDeg = FMath::Clamp(Sanitized.GridFinYMCommandDeg, -Limits.MaxGridFinDeg, Limits.MaxGridFinDeg);

	return Sanitized;
}

FSuperHeavyCommandSaturation ComputeSaturation(
	const FSuperHeavyActuatorCommand& RawCommand,
	const FSuperHeavyActuatorCommand& SanitizedCommand)
{
	FSuperHeavyCommandSaturation Saturation;

	Saturation.bOuterThrottleSaturated = Differs(RawCommand.OuterThrottle, SanitizedCommand.OuterThrottle);
	Saturation.bInnerThrottleSaturated = Differs(RawCommand.InnerThrottle, SanitizedCommand.InnerThrottle);
	Saturation.bCenterThrottleSaturated = Differs(RawCommand.CenterThrottle, SanitizedCommand.CenterThrottle);
	Saturation.bInnerGimbalSaturated =
		Differs(RawCommand.InnerGimbalPitchDeg, SanitizedCommand.InnerGimbalPitchDeg)
		|| Differs(RawCommand.InnerGimbalRollDeg, SanitizedCommand.InnerGimbalRollDeg);
	Saturation.bCenterGimbalSaturated =
		Differs(RawCommand.CenterGimbalPitchDeg, SanitizedCommand.CenterGimbalPitchDeg)
		|| Differs(RawCommand.CenterGimbalRollDeg, SanitizedCommand.CenterGimbalRollDeg);
	Saturation.bGridFinSaturated =
		Differs(RawCommand.GridFinXPCommandDeg, SanitizedCommand.GridFinXPCommandDeg)
		|| Differs(RawCommand.GridFinXMCommandDeg, SanitizedCommand.GridFinXMCommandDeg)
		|| Differs(RawCommand.GridFinYMCommandDeg, SanitizedCommand.GridFinYMCommandDeg);

	Saturation.bAnySaturated =
		Saturation.bOuterThrottleSaturated
		|| Saturation.bInnerThrottleSaturated
		|| Saturation.bCenterThrottleSaturated
		|| Saturation.bInnerGimbalSaturated
		|| Saturation.bCenterGimbalSaturated
		|| Saturation.bGridFinSaturated;

	return Saturation;
}
}

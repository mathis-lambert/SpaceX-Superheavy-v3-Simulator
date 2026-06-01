#include "Control/SuperHeavyActuatorCommandUtils.h"

namespace
{
bool Differs(double A, double B)
{
	constexpr double Tolerance = 1.0e-6;
	return !FMath::IsNearlyEqual(A, B, Tolerance);
}

const FSuperHeavyEngineActuatorCommand* FindEngineCommand(const TArray<FSuperHeavyEngineActuatorCommand>& Commands, FName EngineId)
{
	return Commands.FindByPredicate([EngineId](const FSuperHeavyEngineActuatorCommand& Command)
	{
		return Command.EngineId == EngineId;
	});
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

	for (FSuperHeavyEngineActuatorCommand& EngineCommand : Sanitized.EngineCommands)
	{
		if (EngineCommand.bApplyThrottle)
		{
			EngineCommand.Throttle = FMath::Clamp(EngineCommand.Throttle, Limits.MinThrottle, Limits.MaxThrottle);
		}
		else
		{
			EngineCommand.Throttle = 0.0;
		}

		if (EngineCommand.bApplyGimbal)
		{
			EngineCommand.GimbalPitchDeg = FMath::Clamp(EngineCommand.GimbalPitchDeg, -Limits.MaxGimbalDeg, Limits.MaxGimbalDeg);
			EngineCommand.GimbalRollDeg = FMath::Clamp(EngineCommand.GimbalRollDeg, -Limits.MaxGimbalDeg, Limits.MaxGimbalDeg);
		}
		else
		{
			EngineCommand.GimbalPitchDeg = 0.0;
			EngineCommand.GimbalRollDeg = 0.0;
		}
	}

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

	for (const FSuperHeavyEngineActuatorCommand& RawEngineCommand : RawCommand.EngineCommands)
	{
		const FSuperHeavyEngineActuatorCommand* SanitizedEngineCommand = FindEngineCommand(SanitizedCommand.EngineCommands, RawEngineCommand.EngineId);
		if (!SanitizedEngineCommand)
		{
			continue;
		}

		Saturation.bEngineThrottleSaturated |=
			RawEngineCommand.bApplyThrottle
			&& Differs(RawEngineCommand.Throttle, SanitizedEngineCommand->Throttle);
		Saturation.bEngineGimbalSaturated |=
			RawEngineCommand.bApplyGimbal
			&& (Differs(RawEngineCommand.GimbalPitchDeg, SanitizedEngineCommand->GimbalPitchDeg)
				|| Differs(RawEngineCommand.GimbalRollDeg, SanitizedEngineCommand->GimbalRollDeg));
	}

	Saturation.bGridFinSaturated =
		Differs(RawCommand.GridFinXPCommandDeg, SanitizedCommand.GridFinXPCommandDeg)
		|| Differs(RawCommand.GridFinXMCommandDeg, SanitizedCommand.GridFinXMCommandDeg)
		|| Differs(RawCommand.GridFinYMCommandDeg, SanitizedCommand.GridFinYMCommandDeg);

	Saturation.bAnySaturated =
		Saturation.bEngineThrottleSaturated
		|| Saturation.bEngineGimbalSaturated
		|| Saturation.bGridFinSaturated;

	return Saturation;
}
}

#pragma once

#include "CoreMinimal.h"
#include "GNC/SuperHeavyGncTypes.h"

namespace SuperHeavyActuatorCommandUtils
{
FSuperHeavyActuatorCommand Sanitize(
	const FSuperHeavyActuatorCommand& Command,
	const FSuperHeavyEngineGroupConfig& OuterEngines,
	const FSuperHeavyEngineGroupConfig& InnerEngines,
	const FSuperHeavyEngineGroupConfig& CenterEngines,
	const FSuperHeavyActuatorLimits& Limits);

FSuperHeavyCommandSaturation ComputeSaturation(
	const FSuperHeavyActuatorCommand& RawCommand,
	const FSuperHeavyActuatorCommand& SanitizedCommand);
}

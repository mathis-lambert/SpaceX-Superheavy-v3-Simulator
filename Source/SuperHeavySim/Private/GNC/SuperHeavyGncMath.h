#pragma once

#include "CoreMinimal.h"
#include "GNC/SuperHeavyGncTypes.h"

namespace SuperHeavyGncMath
{
double GetBodyAxisValue(const FVector& Vector, ESuperHeavyBodyAxis Axis);
FVector ComputeAttitudeErrorBodyDeg(const FQuat& CurrentWorldQuat, const FQuat& TargetWorldQuat);
double EstimateGroupThrustN(const FSuperHeavyEngineGroupConfig& Group, double Throttle);
double EstimateCommandedThrustN(
	const FSuperHeavyEngineGroupConfig& OuterEngines,
	const FSuperHeavyEngineGroupConfig& InnerEngines,
	const FSuperHeavyEngineGroupConfig& CenterEngines,
	const FSuperHeavyActuatorCommand& Command);
}

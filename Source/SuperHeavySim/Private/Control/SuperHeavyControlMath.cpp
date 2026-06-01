#include "Control/SuperHeavyControlMath.h"

namespace SuperHeavyControlMath
{
double GetBodyAxisValue(const FVector& Vector, ESuperHeavyBodyAxis Axis)
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

FVector ComputeAttitudeErrorBodyDeg(const FQuat& CurrentWorldQuat, const FQuat& TargetWorldQuat)
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

	const FVector ErrorWorldDeg = ErrorAxisWorld.GetSafeNormal() * FMath::RadiansToDegrees(ErrorAngleRad);
	return CurrentWorldQuat.Inverse().RotateVector(ErrorWorldDeg);
}

double EstimateGroupThrustN(const FSuperHeavyEngineGroupConfig& Group, double Throttle)
{
	return Group.GetMaxThrustN() * FMath::Clamp(Throttle, 0.0, 1.0);
}

double EstimateCommandedThrustN(
	const FSuperHeavyEngineGroupConfig& OuterEngines,
	const FSuperHeavyEngineGroupConfig& InnerEngines,
	const FSuperHeavyEngineGroupConfig& CenterEngines,
	const FSuperHeavyActuatorCommand& Command)
{
	return EstimateGroupThrustN(OuterEngines, Command.OuterThrottle)
		+ EstimateGroupThrustN(InnerEngines, Command.InnerThrottle)
		+ EstimateGroupThrustN(CenterEngines, Command.CenterThrottle);
}
}

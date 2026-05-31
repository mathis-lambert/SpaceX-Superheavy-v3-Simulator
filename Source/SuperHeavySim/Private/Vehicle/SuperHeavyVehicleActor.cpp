#include "Vehicle/SuperHeavyVehicleActor.h"

#include "Logging/SuperHeavyLog.h"

ASuperHeavyVehicleActor::ASuperHeavyVehicleActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ConfigureDefaultActuatorIds();
}

void ASuperHeavyVehicleActor::ApplyActuatorCommand_Implementation(const FSuperHeavyActuatorCommand& Command)
{
	if (Command.bApplyOuterThrottle)
	{
		ApplyThrottleToGroup(OuterEngineIds, Command.OuterThrottle);
	}
	if (Command.bApplyInnerThrottle)
	{
		ApplyThrottleToGroup(InnerEngineIds, Command.InnerThrottle);
	}
	if (Command.bApplyCenterThrottle)
	{
		ApplyThrottleToGroup(CenterEngineIds, Command.CenterThrottle);
	}

	if (Command.bApplyInnerGimbal)
	{
		ApplyGimbalToGroup(InnerEngineIds, Command.InnerGimbalPitchDeg, Command.InnerGimbalRollDeg);
	}
	if (Command.bApplyCenterGimbal)
	{
		ApplyGimbalToGroup(CenterEngineIds, Command.CenterGimbalPitchDeg, Command.CenterGimbalRollDeg);
	}

	if (Command.bApplyGridFins)
	{
		SetGridFinAngleCommand(GridFinXPId, Command.GridFinXPCommandDeg);
		SetGridFinAngleCommand(GridFinXMId, Command.GridFinXMCommandDeg);
		SetGridFinAngleCommand(GridFinYMId, Command.GridFinYMCommandDeg);
	}
}

void ASuperHeavyVehicleActor::ConfigureDefaultActuatorIds()
{
	OuterEngineIds.Reset();
	for (int32 Index = 1; Index <= 20; ++Index)
	{
		OuterEngineIds.Add(FName(*FString::Printf(TEXT("R%02d"), Index)));
	}

	InnerEngineIds.Reset();
	for (int32 Index = 1; Index <= 10; ++Index)
	{
		InnerEngineIds.Add(FName(*FString::Printf(TEXT("RGI%02d"), Index)));
	}

	CenterEngineIds = { TEXT("RGC01"), TEXT("RGC02"), TEXT("RGC03") };

	GridFinXPId = TEXT("GF_XP");
	GridFinXMId = TEXT("GF_XM");
	GridFinYMId = TEXT("GF_YM");
}

void ASuperHeavyVehicleActor::SetEngineThrottleCommand_Implementation(FName EngineId, double Throttle)
{
	if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledThrottleCommand)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyVehicleActor: SetEngineThrottleCommand is not implemented by %s."), *GetName());
		bWarnedUnhandledThrottleCommand = true;
	}
}

void ASuperHeavyVehicleActor::SetEngineGimbalCommand_Implementation(FName EngineId, double PitchDeg, double RollDeg)
{
	if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledGimbalCommand)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyVehicleActor: SetEngineGimbalCommand is not implemented by %s."), *GetName());
		bWarnedUnhandledGimbalCommand = true;
	}
}

void ASuperHeavyVehicleActor::SetGridFinAngleCommand_Implementation(FName GridFinId, double AngleDeg)
{
	if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledGridFinCommand)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyVehicleActor: SetGridFinAngleCommand is not implemented by %s."), *GetName());
		bWarnedUnhandledGridFinCommand = true;
	}
}

void ASuperHeavyVehicleActor::ApplyThrottleToGroup(const TArray<FName>& EngineIds, double Throttle)
{
	for (const FName EngineId : EngineIds)
	{
		SetEngineThrottleCommand(EngineId, Throttle);
	}
}

void ASuperHeavyVehicleActor::ApplyGimbalToGroup(const TArray<FName>& EngineIds, double PitchDeg, double RollDeg)
{
	for (const FName EngineId : EngineIds)
	{
		SetEngineGimbalCommand(EngineId, PitchDeg, RollDeg);
	}
}

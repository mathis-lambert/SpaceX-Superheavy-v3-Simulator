#include "Vehicle/SuperHeavyVehicleActor.h"

#include "Logging/SuperHeavyLog.h"

ASuperHeavyVehicleActor::ASuperHeavyVehicleActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ConfigureDefaultActuatorIds();
}

void ASuperHeavyVehicleActor::ApplyActuatorCommand_Implementation(const FSuperHeavyActuatorCommand& Command)
{
	for (const FSuperHeavyEngineActuatorCommand& EngineCommand : Command.EngineCommands)
	{
		if (EngineCommand.bApplyThrottle)
		{
			SetEngineThrottleCommand(EngineCommand.EngineId, EngineCommand.Throttle);
		}
		if (EngineCommand.bApplyGimbal)
		{
			SetEngineGimbalCommand(EngineCommand.EngineId, EngineCommand.GimbalPitchDeg, EngineCommand.GimbalRollDeg);
		}
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

void ASuperHeavyVehicleActor::SetActiveCameraByIndexCommand_Implementation(int32 CameraIndex)
{
	if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledCameraCommand)
	{
		UE_LOG(LogSuperHeavyGnc, Warning, TEXT("SuperHeavyVehicleActor: SetActiveCameraByIndexCommand is not implemented by %s."), *GetName());
		bWarnedUnhandledCameraCommand = true;
	}
}

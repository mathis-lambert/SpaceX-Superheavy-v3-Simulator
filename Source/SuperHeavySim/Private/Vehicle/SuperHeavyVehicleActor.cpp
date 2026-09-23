#include "Vehicle/SuperHeavyVehicleActor.h"

#include "Recovery/Shared/RecoveryLog.h"

ASuperHeavyVehicleActor::ASuperHeavyVehicleActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASuperHeavyVehicleActor::SetEngineThrottleCommand_Implementation(FName EngineId, double Throttle)
{
    if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledThrottleCommand)
    {
        UE_LOG(LogRecovery, Warning, TEXT("SuperHeavyVehicleActor: SetEngineThrottleCommand is not implemented by %s."),
               *GetName());
        bWarnedUnhandledThrottleCommand = true;
    }
}

void ASuperHeavyVehicleActor::SetEngineGimbalCommand_Implementation(FName EngineId, double PitchDeg, double RollDeg)
{
    if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledGimbalCommand)
    {
        UE_LOG(LogRecovery, Warning, TEXT("SuperHeavyVehicleActor: SetEngineGimbalCommand is not implemented by %s."),
               *GetName());
        bWarnedUnhandledGimbalCommand = true;
    }
}

void ASuperHeavyVehicleActor::SetGridFinAngleCommand_Implementation(FName GridFinId, double AngleDeg)
{
    if (bWarnOnUnhandledActuatorCommands && !bWarnedUnhandledGridFinCommand)
    {
        UE_LOG(LogRecovery, Warning, TEXT("SuperHeavyVehicleActor: SetGridFinAngleCommand is not implemented by %s."),
               *GetName());
        bWarnedUnhandledGridFinCommand = true;
    }
}

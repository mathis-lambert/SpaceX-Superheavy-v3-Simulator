#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SuperHeavyVehicleActor.generated.h"

UCLASS(Blueprintable)
class SUPERHEAVYSIM_API ASuperHeavyVehicleActor : public AActor
{
    GENERATED_BODY()

public:
    ASuperHeavyVehicleActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Super Heavy|Vehicle API")
    bool bWarnOnUnhandledActuatorCommands = true;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Super Heavy|Vehicle API")
    void SetEngineThrottleCommand(FName EngineId, double Throttle);
    virtual void SetEngineThrottleCommand_Implementation(FName EngineId, double Throttle);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Super Heavy|Vehicle API")
    void SetEngineGimbalCommand(FName EngineId, double PitchDeg, double RollDeg);
    virtual void SetEngineGimbalCommand_Implementation(FName EngineId, double PitchDeg, double RollDeg);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Super Heavy|Vehicle API")
    void SetGridFinAngleCommand(FName GridFinId, double AngleDeg);
    virtual void SetGridFinAngleCommand_Implementation(FName GridFinId, double AngleDeg);

protected:
    UPROPERTY(Transient)
    bool bWarnedUnhandledThrottleCommand = false;

    UPROPERTY(Transient)
    bool bWarnedUnhandledGimbalCommand = false;

    UPROPERTY(Transient)
    bool bWarnedUnhandledGridFinCommand = false;
};

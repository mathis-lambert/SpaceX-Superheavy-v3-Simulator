#pragma once

#include "Autopilot/SuperHeavyAutopilotTypes.h"
#include "Engine/DataAsset.h"
#include "SuperHeavyMissionProfile.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SUPERHEAVYSIM_API USuperHeavyMissionProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId = TEXT("SuperHeavyMission");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FSuperHeavyMissionTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Initial State")
	bool bResetVehicleToLaunchTransformOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Initial State", meta = (EditCondition = "bResetVehicleToLaunchTransformOnStart"))
	FVector InitialLinearVelocityMps = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Initial State", meta = (EditCondition = "bResetVehicleToLaunchTransformOnStart"))
	FVector InitialAngularVelocityDegPerSec = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	ESuperHeavyFlightPhase InitialPhase = ESuperHeavyFlightPhase::GroundIdle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	ESuperHeavyFlightPhase AbortPhase = ESuperHeavyFlightPhase::Abort;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TArray<FSuperHeavyFlightPhaseConfig> Phases;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	bool FindPhaseConfig(ESuperHeavyFlightPhase Phase, FSuperHeavyFlightPhaseConfig& OutConfig) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission")
	bool IsPhaseConfigured(ESuperHeavyFlightPhase Phase) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Validation")
	FSuperHeavyMissionValidationResult ValidateMission() const;
};

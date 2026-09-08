#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SuperHeavyRecoveryProfile.generated.h"
/** SI units. Specifications and estimates are documented in Docs/FLIGHT_MODEL.md. */
UCLASS(BlueprintType)
class SUPERHEAVYSIM_API USuperHeavyRecoveryProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") FString MissionName = TEXT("V3 / suborbital return to launch site");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") FVector LaunchOffsetM = FVector(140,0,12);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") double ApogeeM = 95000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") double AscentDurationS = 142;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") double AscentPitchDeg = 58;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") double AscentMaxAccelerationMps2 = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission") double TimeoutSeconds = 650;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mass | Estimated") double DryMassKg = 210000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mass | Published capacity") double PropellantMassKg = 3650000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mass | Estimated") double UpperStageMassKg = 1750000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tanks | Estimated") double MixtureRatio = 3.6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tanks | Estimated") double OxygenDensityKgM3 = 1141;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tanks | Estimated") double MethaneDensityKgM3 = 422;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tanks | Estimated") double OxygenTankBottomM = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tanks | Estimated") double MethaneTankBottomM = 42.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Propulsion | Published") double EngineThrustN = 2451662.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Propulsion | Estimated") double SpecificImpulseSeaLevelS = 327;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Propulsion | Estimated") double SpecificImpulseVacuumS = 350;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Propulsion | Estimated") double MinimumThrottle = 0.3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Propulsion | Estimated") double LandingReserveKg = 75000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance | Estimated") double BoostbackReserveKg = 400000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance | Estimated") double LandingDriftCorrectionS = -8;
    // Estimated braking reserve in front of the opening; measured in tower-local +X.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance | Approach", meta=(ClampMin="400", ClampMax="5000")) double FrontReturnOffsetM = 1400;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance | Estimated", meta=(ClampMin="0", ClampMax="60")) double LandingWindLeadS = 18;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance") double LandingIgnitionCeilingM = 4500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators") double ThrottleTimeConstant = 0.25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators | Estimated") double EngineShutdownTimeS = 0.35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators") double MaxGimbalDeg = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators | Estimated") double GimbalRateDegS = 45;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators | Estimated") double ReactionValveTimeConstantS = 0.04;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Upper stage | Estimated") double UpperStageDryMassKg = 130000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Upper stage | Estimated") double UpperStageEngineThrustN = 3000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Upper stage | Estimated") double UpperStageIspS = 370;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators | Estimated") double ReactionControlTorqueNm = 12000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actuators | Estimated") double ReactionControlPropellantKg = 7000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double DragAreaM2 = 64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double AxialDragCoefficient = 0.8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double TailFirstDragCoefficient = 1.7;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double GridFinDragCoefficient = 1.2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double BodySideAreaM2 = 630;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double BodyNormalCoefficient = 1.1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double GridFinAreaM2 = 8.2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics | Estimated") double GridFinLiftSlope = 2.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics") double GridFinMaxAngleDeg = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics") double GridFinRateDegS = 45;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aerodynamics") double MaxEntryAngleDeg = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance") double MaxTiltDeg = 15;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance") double LandingDecelerationMps2 = 19;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance") double LandingBurnMarginM = 350;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment") double SeaLevelTemperatureOffsetK = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground systems | Estimated") double ConditioningVentKgS = 1.8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground systems | Estimated") double ConditioningJetSpeedMps = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") double CaptureRadiusM = 0.35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") double CaptureSpeedMps = 0.6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") double CaptureTiltDeg = 1.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") double CaptureHeadingToleranceDeg = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") double CaptureDwellSeconds = 0.6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") double CaptureHeadingDeg = 90;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment") FVector WindVelocityMps = FVector(0,4,0);
    // Opposed fittings on source-mesh +/-X; tower rails along tower-local X.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") FVector CatchLugPlusM = FVector(4.99,0.0079,62.7978);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture") FVector CatchLugMinusM = FVector(-4.99,0.0079,62.7978);
    double LaunchMassKg() const { return DryMassKg+PropellantMassKg+UpperStageMassKg+ReactionControlPropellantKg; }
    bool Validate(FString& Reason) const;
};

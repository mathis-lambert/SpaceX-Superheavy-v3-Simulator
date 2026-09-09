#pragma once
#include "CoreMinimal.h"
#include "RecoveryTowerMechanics.generated.h"

/** Engineering estimates, not manufacturer data. SI units at this boundary. */
USTRUCT(BlueprintType)
struct FRecoveryTowerMechanics
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mass", meta=(ClampMin="1000")) double ArmMassKg=65000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mass", meta=(ClampMin="100")) double RailMassKg=2500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hydraulics", meta=(ClampMin="0")) double MotorTorqueNm=12000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hydraulics", meta=(ClampMin="0")) double MotorStiffnessNmRad=600000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hydraulics", meta=(ClampMin="0")) double MotorDampingNmsRad=180000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rails", meta=(ClampMin="1000")) double RailStiffnessNm=50000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rails", meta=(ClampMin="0")) double RailDampingNsm=4500000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rails", meta=(ClampMin="0")) double RailForceLimitN=7000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rails", meta=(ClampMin="0.01")) double RailTravelM=.35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strength", meta=(ClampMin="0")) double RailBreakForceN=9000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strength", meta=(ClampMin="0")) double HingeBreakForceN=12000000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strength", meta=(ClampMin="0")) double HingeBreakTorqueNm=220000000;
};

/** Persistent observations sampled after every Chaos step, including break events. */
struct FRecoveryTowerState
{
    FVector2D RailLoadN=FVector2D::ZeroVector,PeakRailLoadN=FVector2D::ZeroVector;
    FVector2D PeakHingeTorqueNm=FVector2D::ZeroVector;
    uint8 BrokenRails=0,BrokenHinges=0;
};

namespace RecoveryTowerGeometry
{
    constexpr double FrameDropM=.4;
    constexpr double RailCentreHeightM=.96+FrameDropM;
    constexpr double HalfArmM=13.;
    constexpr double ClosedGapM=5.15;
    constexpr double GapTravelM=4.85;
    inline double AngleDeg(double Closure,double CaptureLeverM)
    { return FMath::RadiansToDegrees(FMath::Atan(GapTravelM*(1-FMath::Clamp(Closure,0.,1.))/CaptureLeverM)); }
    inline double Closure(double AngleDeg,double CaptureLeverM)
    { return FMath::Clamp(1-FMath::Tan(FMath::DegreesToRadians(FMath::Abs(AngleDeg)))*CaptureLeverM/GapTravelM,0.,1.); }
}

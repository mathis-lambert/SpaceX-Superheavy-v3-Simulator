#pragma once
#include "CoreMinimal.h"

/** Observed solver contacts. No support force or constraint is added here. */
struct FRecoveryRailSupport
{
    uint8 CurrentMask=0,EverMask=0;
    FVector2D ImpulseNs=FVector2D::ZeroVector;
    FVector ReactionImpulseWorldNs[2]={FVector::ZeroVector,FVector::ZeroVector};
    uint64 SolverSamples=0;
    int32 Count() const { return int32((CurrentMask&1)!=0)+int32((CurrentMask&2)!=0); }
};

namespace RecoveryContactGeometry
{
    constexpr double RailHalfLengthM=13.;
    // The 26 m rail starts at the carriage, clear of the tower's front columns.
    constexpr double RailCentreOffsetM=-4.;
    constexpr double FittingHalfLengthM=.9;
    inline bool OnUsableRailSpan(double AlongM)
    {
        // Leave the full fitting and a half-metre end margin on the rail.
        return FMath::Abs(AlongM-RailCentreOffsetM)<RailHalfLengthM-FittingHalfLengthM-.5;
    }
    inline bool AtFitting(const FVector& OffsetM)
    {
        return FMath::Abs(OffsetM.X)<FittingHalfLengthM && FMath::Abs(OffsetM.Y)<0.8 && FMath::Abs(OffsetM.Z)<0.5;
    }
}

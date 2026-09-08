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
    inline bool AtFitting(const FVector& OffsetM)
    {
        return FMath::Abs(OffsetM.X)<0.9 && FMath::Abs(OffsetM.Y)<0.8 && FMath::Abs(OffsetM.Z)<0.5;
    }
}

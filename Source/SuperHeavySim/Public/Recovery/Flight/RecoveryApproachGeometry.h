#pragma once
#include "CoreMinimal.h"

/** Tower-local approach policy. These limits accept/reject flight paths;
 * they never constrain or modify a rigid body. +X faces the arm opening. */
namespace RecoveryApproach
{
    constexpr double CorridorSlope=.25;
    // 2 m centreline allowance fits inside the open 20 m arm gap with
    // a 9 m diameter body; contact alignment is checked separately at 0.5 m.
    constexpr double CorridorHalfWidthM=2.;
    constexpr double FinalApproachHeightM=350.;

    inline double CorridorMargin(const FVector& FittingPosition,const FVector& Target,const FQuat& TowerRotation)
    {
        const FVector Local=TowerRotation.UnrotateVector(FittingPosition-Target);
        return FMath::Min(Local.X+.6,CorridorHalfWidthM+FMath::Max(0.,Local.X)*CorridorSlope-FMath::Abs(Local.Y));
    }

    inline double MastFrontMargin(const FVector& Base,const FVector& Up,const FVector& TowerPosition,const FQuat& TowerRotation)
    {
        const FVector B=TowerRotation.UnrotateVector(Base-TowerPosition);
        const FVector T=TowerRotation.UnrotateVector(Base+Up*70.88-TowerPosition);
        // Entire 9 m diameter body stays in front of the mast's projection,
        // including at altitudes where flying over it would avoid collision.
        return FMath::Min(B.X,T.X)-4.5-7.;
    }
}

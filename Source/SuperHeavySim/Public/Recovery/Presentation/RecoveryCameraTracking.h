#pragma once
#include "CoreMinimal.h"

/** Presentation state only: contact jitter must never define a flight heading. */
struct FRecoveryChaseTracking
{
    FVector Direction=FVector::UpVector;
    bool bTracking=false;

    FVector Update(const FVector& VelocityMps,double Dt)
    {
        const double Speed=VelocityMps.Size();
        if(Speed>5.)bTracking=true;
        else if(Speed<2.)bTracking=false;
        if(bTracking && Dt>0)
        {
            const FVector Target=VelocityMps/Speed;
            const FQuat Turn=FQuat::FindBetweenNormals(Direction,Target);
            const double Angle=Turn.GetAngle();
            const double Fraction=Angle>1.e-8 ? FMath::Min(1-FMath::Exp(-Dt/.65),FMath::DegreesToRadians(65.)*Dt/Angle) : 1.;
            Direction=FQuat::Slerp(FQuat::Identity,Turn,Fraction).RotateVector(Direction).GetSafeNormal();
        }
        return Direction;
    }
};

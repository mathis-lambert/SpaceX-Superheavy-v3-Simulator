#pragma once
#include "CoreMinimal.h"

/** Smooth the user's composition, never the rocket's world position. */
struct FRecoveryOrbitInput
{
    double TargetYaw=0,TargetPitch=0,Yaw=0,Pitch=0;
    void Add(double X,double Y)
    {
        TargetYaw=FMath::UnwindDegrees(TargetYaw+X);
        TargetPitch=FMath::Clamp(TargetPitch+Y,-75.,75.);
    }
    void Step(double WallSeconds)
    {
        const double Alpha=1-FMath::Exp(-FMath::Max(0.,WallSeconds)/.045);
        Yaw=FMath::UnwindDegrees(Yaw+FMath::FindDeltaAngleDegrees(Yaw,TargetYaw)*Alpha);
        Pitch=FMath::Lerp(Pitch,TargetPitch,Alpha);
    }
};

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

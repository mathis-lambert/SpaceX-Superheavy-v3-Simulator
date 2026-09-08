#pragma once

#include "CoreMinimal.h"

/** Physical actuator state in SI units, independent of presentation components. */
struct FRecoveryEngineState
{
    FName Id;
    FVector PositionFromBaseM=FVector::ZeroVector;
    FVector NozzleOffsetBodyM=FVector::ZeroVector;
    FVector DirectionBody=FVector::UpVector;
    double ThrustN=0;
    // The endpoint drives telemetry/VFX; the integral drives fuel and mechanics.
    double StepImpulseNs=0;
    FVector StepForceBodyN=FVector::ZeroVector;
    bool bGimballed=false;
    bool bCentral=false;
};

namespace RecoveryActuators
{
    /** Least-squares lateral engine forces for a body-frame moment request.
     * Actual force limits and gimbal dynamics are applied by each actuator. */
    inline FVector SolveSymmetric(const FVector& C0,const FVector& C1,const FVector& C2,const FVector& B)
    {
        const double Determinant=FVector::DotProduct(C0,FVector::CrossProduct(C1,C2));
        if(FMath::Abs(Determinant)<1.e-9) return FVector::ZeroVector;
        return FVector(FVector::DotProduct(B,FVector::CrossProduct(C1,C2)),
            FVector::DotProduct(B,FVector::CrossProduct(C2,C0)),
            FVector::DotProduct(B,FVector::CrossProduct(C0,C1)))/Determinant;
    }

    inline double FuelLimitedThrust(double RequestedN,double AvailableKg,double IspS,double Dt)
    {
        return Dt>0 && IspS>0 ? FMath::Clamp(RequestedN,0.,FMath::Max(0.,AvailableKg)*IspS*9.80665/Dt) : 0.;
    }
}

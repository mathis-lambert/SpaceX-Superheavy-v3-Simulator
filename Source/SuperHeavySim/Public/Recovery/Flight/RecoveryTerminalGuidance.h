#pragma once
#include "Recovery/Flight/RecoveryDynamicsModel.h"

/** Centre-of-mass coordinates in SI units. This is a prediction, never a body pose. */
struct FRecoveryTerminalInput
{
    FVector PositionM=FVector::ZeroVector,VelocityMps=FVector::ZeroVector;
    FVector AccelerationMps2=FVector::ZeroVector;
    FVector TargetM=FVector::ZeroVector,TargetVelocityMps=FVector(0,0,-.35);
    FVector UpWorld=FVector::UpVector,WindMps=FVector::ZeroVector;
    FVector HeadingWorld=FVector::RightVector;
    FVector TowerWorldM=FVector::ZeroVector;
    FVector FittingMidFromBaseM=FVector(0,.0079,62.7978),TargetFittingWorldM=FVector(24,.0079,82.7978);
    FQuat TowerRotation=FQuat::Identity;
    double TowerHeightM=105,CaptureBaseHeightM=20,CentreFromBaseM=30;
    double MassKg=0,FuelKg=0,CoreThrustN=0,LandingThrustN=0,IspS=0;
    double MaxTiltDeg=15;
};

struct FRecoveryTerminalPlan
{
    bool bFeasible=false;
    double HorizonS=0,ElapsedS=0,EstimatedFuelKg=0,PeakThrustN=0,PeakTiltDeg=0;
    int32 Candidates=0;
    int32 ThrustRejected=0,AttitudeRejected=0,ClearanceRejected=0,FuelRejected=0;
    // Rejected candidate geometry: mast projection, front corridor, mast
    // collision, arm opening, contact alignment, floor (bits 0 through 5).
    uint32 ClearanceReasons=0;
    FVector PositionM=FVector::ZeroVector,VelocityMps=FVector::ZeroVector;
    FVector InitialAccelerationMps2=FVector::ZeroVector;
    FVector CubicMps3=FVector::ZeroVector,QuarticMps4=FVector::ZeroVector,QuinticMps5=FVector::ZeroVector;
    FVector CrossrangeCorrectionM=FVector::ZeroVector;
    FVector PositionAt(double TimeS) const;
    FVector VelocityAt(double TimeS) const;
    FVector AccelerationAt(double TimeS) const;
};

namespace RecoveryTerminalGuidance
{
    /** Sampled point-mass feasibility with estimated body aero and changing mass.
     * The actuator model still independently enforces thrust, slew and fuel limits. */
    FRecoveryTerminalPlan Plan(const FRecoveryTerminalInput& Input,const FRecoveryDynamicsConfiguration& Config);
    bool TryRequiredThrustAcceleration(const FVector& NetAcceleration,const FVector& VelocityMps,
        const FVector& PositionM,double MassKg,const FVector& WindMps,const FRecoveryDynamicsConfiguration& Config,
        const FVector& InitialUpWorld,FVector& ThrustAcceleration);
}

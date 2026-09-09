#pragma once
#include "Recovery/Flight/RecoveryDynamicsModel.h"

/** A vertical reachability estimate, not an integrator for the real vehicle. */
struct FRecoveryLandingPrediction
{
    bool bFeasible=false;
    double DistanceM=0,DurationS=0,FuelKg=0;
    double CoreThrustN=0,LandingThrustN=0;
};

namespace RecoveryLanding
{
    /** Bang-bang lower bound with the current signed closing velocity. */
    double MinimumTransferTime(double SignedDistanceM,double VelocityMps,double AccelerationMps2);
    /** Project 13-to-3 braking with changing air, mass, finite valves and failed
     * engines. Reserve core authority for attitude and the final lateral transfer. */
    FRecoveryLandingPrediction Predict(const FRecoveryDynamicsConfiguration& Config,
        const FRecoveryDynamicsState& Dynamics,double AltitudeM,double VerticalSpeedMps,
        double UpProjection,double MaximumDecelerationMps2,int32 FailedEngine);
}

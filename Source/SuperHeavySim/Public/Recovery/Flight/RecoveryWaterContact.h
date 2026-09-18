#pragma once
#include "Recovery/Flight/RecoveryDynamicsModel.h"

namespace RecoveryWaterContact
{
    // Spherical quadrature cells approximate the enclosed hull. No pose clamps,
    // velocity replacement, explosion or full-fluid simulation.
    double Apply(const FRecoveryBodyKinematics& Body,const RecoveryMass::FProperties& Mass,
        double OriginFromBaseM,double HullBottomM,double HullLengthM,double RadiusM,
        const FRecoveryWaterMap* Map,double Dt,TArray<FRecoveryForceSample>& Forces);
}

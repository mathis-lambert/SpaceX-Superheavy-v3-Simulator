#pragma once
#include "CoreMinimal.h"
#include "Recovery/Flight/RecoveryActuators.h"

namespace RecoveryPropulsionVisuals
{
    inline double DeliveredFraction(const TArray<FRecoveryEngineState>& Engines,double RatedThrustN)
    {
        double Sum=0;for(const auto& Engine:Engines)Sum+=FMath::Max(0.,Engine.ThrustN);
        return FMath::Clamp(Sum/FMath::Max(1.,RatedThrustN*33),0.,1.);
    }
    // Hot exhaust remains luminous briefly after chamber thrust decays. This
    // presentation tail never feeds back into delivered thrust or propellant.
    inline double ExhaustEnvelope(double Previous,double Delivered,double Dt)
    { return Delivered>=Previous?Delivered:FMath::Max(Delivered,Previous*FMath::Exp(-FMath::Max(0.,Dt)/.10)); }
}

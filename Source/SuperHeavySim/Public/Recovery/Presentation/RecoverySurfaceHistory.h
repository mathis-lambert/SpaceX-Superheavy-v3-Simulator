#pragma once
#include "CoreMinimal.h"

/** Optical surface history, never a force or a flight-control input. */
struct FRecoverySurfaceHistory
{
    double Wetness=0,Residue=0;
    uint32 Generation=MAX_uint32;
    void Update(uint32 MissionGeneration,double Dt,double Deluge,double GroundExhaust)
    {
        if(Generation!=MissionGeneration){Wetness=Residue=0;Generation=MissionGeneration;}
        if(!FMath::IsFinite(Dt) || Dt<=0)return;
        // Analytic first-order integration remains consistent under time scaling.
        const double Inflow=FMath::Clamp(Deluge,0.,1.)*.18;
        const double Drying=1./240.;
        const double Rate=Inflow+Drying;
        const double Target=Inflow/Rate;
        Wetness=Target+(Wetness-Target)*FMath::Exp(-Rate*Dt);
        const double Deposit=FMath::Clamp(GroundExhaust,0.,1.)*.012;
        Residue=1-(1-Residue)*FMath::Exp(-Deposit*Dt);
    }
};

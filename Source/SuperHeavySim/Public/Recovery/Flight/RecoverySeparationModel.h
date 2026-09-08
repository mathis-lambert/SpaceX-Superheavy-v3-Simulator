#pragma once
#include "Recovery/Flight/RecoveryDynamicsModel.h"
#include "Recovery/Shared/FlightGeometry.h"

namespace RecoverySeparation
{
    struct FState
    {
        FRecoveryBodyKinematics Booster,UpperStage;
        double LinearMomentumRelativeError=0,AngularMomentumRelativeError=0;
    };

    /** Inherit one live rigid-stack velocity field at its two new centres of mass.
     * Only used once to create independent bodies; no separation kick is added. */
    inline FState Split(const FRecoveryBodyKinematics& Stack,const RecoveryMass::FProperties& Before,
        const RecoveryMass::FProperties& BoosterMass,const RecoveryMass::FProperties& StageMass)
    {
        FState Result;Result.Booster=Result.UpperStage=Stack;
        const FQuat Q=Stack.Rotation;
        const FVector Base=Stack.OriginM-Q.GetUpVector()*FlightGeometry::BoosterBaseOffsetM;
        const FVector BeforeCentre=Base+Q.GetUpVector()*Before.CentreFromBaseM;
        const FVector BoosterCentre=Base+Q.GetUpVector()*BoosterMass.CentreFromBaseM;
        const FVector StageCentre=Base+Q.GetUpVector()*(FlightGeometry::UpperStageBaseHeightM+StageMass.CentreFromBaseM);
        Result.UpperStage.OriginM=StageCentre;
        Result.Booster.VelocityMps+=FVector::CrossProduct(Stack.AngularVelocityWorldRadS,BoosterCentre-BeforeCentre);
        Result.UpperStage.VelocityMps+=FVector::CrossProduct(Stack.AngularVelocityWorldRadS,StageCentre-BeforeCentre);
        const FVector P0=Stack.VelocityMps*Before.MassKg;
        const FVector PB=Result.Booster.VelocityMps*BoosterMass.MassKg,PS=Result.UpperStage.VelocityMps*StageMass.MassKg;
        const FVector Omega=Q.UnrotateVector(Stack.AngularVelocityWorldRadS);
        const FVector L0=Q.RotateVector(Before.InertiaKgM2*Omega);
        const FVector L1=Q.RotateVector((BoosterMass.InertiaKgM2+StageMass.InertiaKgM2)*Omega)+
            FVector::CrossProduct(BoosterCentre-BeforeCentre,PB)+FVector::CrossProduct(StageCentre-BeforeCentre,PS);
        Result.LinearMomentumRelativeError=(PB+PS-P0).Size()/FMath::Max(1.,P0.Size());
        Result.AngularMomentumRelativeError=(L1-L0).Size()/FMath::Max(1.,L0.Size());
        return Result;
    }
}

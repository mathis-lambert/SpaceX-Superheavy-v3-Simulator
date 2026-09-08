#pragma once
#include "Recovery/Flight/RecoveryDynamicsModel.h"

struct FRecoveryUpperStageConfiguration
{
    double DryMassKg=0,InitialFuelKg=0,EngineThrustN=0,SpecificImpulseS=0;
    double TemperatureOffsetK=0;
    FVector SurfaceWindMps=FVector::ZeroVector;
};

/** Independent stage state, published by the solver with the booster snapshot. */
struct FRecoveryUpperStageState
{
    FRecoveryBodyKinematics Body;
    RecoveryMass::FProperties Mass;
    TArray<FRecoveryForceSample,TInlineAllocator<8>> Forces;
    double PropellantKg=0,FuelConsumedKg=0,ThrustN=0,DeliveredImpulseNs=0,ElapsedS=0;
    double MinimumStepS=TNumericLimits<double>::Max(),MaximumStepS=0;
    uint64 Steps=0;
    bool bSeparated=false;
    double SeparationMassKg=0,SeparationVelocityErrorMps=0;
    double SeparationMomentumRelativeError=0,SeparationAngularMomentumRelativeError=0;
};

/** Fixed, symmetric six-engine stage. Orbital GNC and detailed tanks remain separate work. */
class FRecoveryUpperStageModel
{
public:
    void Reset(const FRecoveryUpperStageConfiguration& Configuration);
    void Step(const FRecoveryBodyKinematics& Body,double WindScale,double Dt);
    const FRecoveryUpperStageState& GetState() const { return State; }
    void RecordSeparation(double BoosterMassKg,double VelocityError,double LinearError,double AngularError)
    {
        State.bSeparated=true;State.SeparationMassKg=BoosterMassKg;State.SeparationVelocityErrorMps=VelocityError;
        State.SeparationMomentumRelativeError=LinearError;State.SeparationAngularMomentumRelativeError=AngularError;
    }
private:
    FRecoveryUpperStageConfiguration Config;
    FRecoveryUpperStageState State;
};

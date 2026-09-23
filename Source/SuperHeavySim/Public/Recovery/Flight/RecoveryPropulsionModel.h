#pragma once

#include "Recovery/Flight/RecoveryActuators.h"

/** Value-only propulsion inputs. No actor, component, world or rendering access. */
struct FRecoveryEngineParameters
{
    double MinimumThrottle=0.3;
    double OpeningTimeConstantS=0.25;
    double ShutdownTimeS=0.35;
    double MaximumGimbalDeg=8;
    double GimbalRateDegS=45;
    double GimbalTimeConstantS=0.08;
};

struct FRecoveryEngineCommand
{
    int32 RequestedCount=0;
    int32 FailedEngine=INDEX_NONE;
    double RequestedThrustN=0;
    double RatedThrustN=0;
    double SpecificImpulseS=0;
};

struct FRecoveryPropulsionStep
{
    double AvailableThrustN=0;
    double DeliveredImpulseNs=0;
    double FuelUsedKg=0;
    FVector ForceBodyN=FVector::ZeroVector;
    FVector MomentBodyNm=FVector::ZeroVector;
};

namespace RecoveryPropulsion
{
    struct FValveStep { double EndThrustN=0; double ImpulseNs=0; };

    /** Exact impulse for a held command and a first-order opening valve, or a
     * finite-rate closing valve. End thrust is never substituted for step mean. */
    FValveStep AdvanceValve(double StartN,double TargetN,double RatedN,
        double OpeningTimeConstantS,double ShutdownTimeS,double Dt);

    FRecoveryPropulsionStep AdvanceEngines(TArray<FRecoveryEngineState>& Engines,
        const FRecoveryEngineParameters& Parameters,const FRecoveryEngineCommand& Command,
        double AvailableFuelKg,double Dt);

    /** Allocate realizable engine moments after updating mass and centre of mass. */
    void AllocateGimbals(TArray<FRecoveryEngineState>& Engines,const FRecoveryEngineParameters& Parameters,
        const FVector& CentreFromBaseM,const FVector& DesiredMomentBodyNm,double Dt,FRecoveryPropulsionStep& Step,
        const FVector& DesiredForceBodyN=FVector::ZeroVector,double TranslationWeight=0);
}

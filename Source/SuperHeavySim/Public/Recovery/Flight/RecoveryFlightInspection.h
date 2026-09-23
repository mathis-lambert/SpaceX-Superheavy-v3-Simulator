#pragma once
#include "CoreMinimal.h"

enum class ERecoveryForceKind : uint8 { Engine, Aerodynamic, GridFin, ReactionJet, Vent, Gravity, Water };

/** SI data captured where forces are applied; presentation cannot write back. */
struct FRecoveryForceSample
{
    ERecoveryForceKind Kind;
    int32 Index;
    FVector PointCm;
    FVector ForceN;
};

struct FRecoveryScheduledFault
{
    int32 Kind=0,Index=0;
    double StartS=0,DurationS=1;
};

/** Deliberate experiment inputs. Defaults preserve the nominal vehicle model. */
struct FRecoveryFlightExperiment
{
    TArray<FRecoveryScheduledFault> Schedule;
    int32 FailedEngine=INDEX_NONE;
    int32 JammedFin=INDEX_NONE;
    double JammedFinAngleDeg=0;
    bool bReactionJetsDisabled=false;
    double AttitudeResponse=1.;
    double WindScale=1.;
    double EngineRestoreTimeS=-1,FinRestoreTimeS=-1,RcsRestoreTimeS=-1;
    // Before liftoff the mission clock can be held or negative. Timed operator
    // faults still expire in simulated seconds while a countdown is held.
    bool AdvanceGroundFaults(double Dt)
    {
        bool Restored=false;
        const auto Expire=[&](double& Remaining,auto& Value,const auto RestoredValue)
        {if(Remaining>=0){Remaining-=Dt;if(Remaining<=0){Remaining=-1;Value=RestoredValue;Restored=true;}}};
        Expire(EngineRestoreTimeS,FailedEngine,INDEX_NONE);
        Expire(FinRestoreTimeS,JammedFin,INDEX_NONE);
        Expire(RcsRestoreTimeS,bReactionJetsDisabled,false);
        return Restored;
    }
    FRecoveryFlightExperiment AtTime(double TimeS) const
    {
        auto Effective=*this;
        if(EngineRestoreTimeS>=0 && TimeS>=EngineRestoreTimeS)Effective.FailedEngine=INDEX_NONE;
        if(FinRestoreTimeS>=0 && TimeS>=FinRestoreTimeS)Effective.JammedFin=INDEX_NONE;
        if(RcsRestoreTimeS>=0 && TimeS>=RcsRestoreTimeS)Effective.bReactionJetsDisabled=false;
        for(const auto& Fault:Schedule)if(TimeS>=Fault.StartS && TimeS<Fault.StartS+Fault.DurationS)
        {
            if(Fault.Kind==1)Effective.FailedEngine=Fault.Index;
            if(Fault.Kind==2){Effective.JammedFin=Fault.Index;Effective.JammedFinAngleDeg=0;}
            if(Fault.Kind==3)Effective.bReactionJetsDisabled=true;
        }
        return Effective;
    }
};

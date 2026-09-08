#pragma once
#include "CoreMinimal.h"

enum class ERecoveryForceKind : uint8 { Engine, Aerodynamic, GridFin, ReactionJet, Vent, Gravity };

/** SI data captured where forces are applied; presentation cannot write back. */
struct FRecoveryForceSample
{
    ERecoveryForceKind Kind;
    int32 Index;
    FVector PointCm;
    FVector ForceN;
};

/** Deliberate experiment inputs. Defaults preserve the nominal vehicle model. */
struct FRecoveryFlightExperiment
{
    int32 FailedEngine=INDEX_NONE;
    int32 JammedFin=INDEX_NONE;
    double JammedFinAngleDeg=0;
    bool bReactionJetsDisabled=false;
    double AttitudeResponse=1.;
    double WindScale=1.;
};

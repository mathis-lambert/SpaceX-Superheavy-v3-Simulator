#include "Recovery/Flight/RecoveryLaunchSequence.h"

void FRecoveryLaunchSequence::Start()
{
    *this = FRecoveryLaunchSequence();
    bRunning = true;
}

void FRecoveryLaunchSequence::Advance(double Dt, bool bSystemsReady, bool bThrustVerified)
{
    if (!bRunning || bAborted || bReleaseRequested || !FMath::IsFinite(Dt) || Dt <= 0.) return;
    if (!bSystemsReady)
    {
        // A loss of readiness after ignition shuts down; a cold vehicle can hold.
        if (RemainingS <= 3.) Abort();
        else bHeld = true;
        return;
    }
    bHeld = false;
    RemainingS = FMath::Max(0., RemainingS - Dt);
    if (RemainingS == 0.)
    {
        if (bThrustVerified) bReleaseRequested = true;
        else Abort();
    }
}

void FRecoveryLaunchSequence::Abort()
{
    bAborted = true;
    bHeld = false;
}

bool FRecoveryLaunchSequence::IsIgnitionCommanded() const
{
    return bRunning && !bHeld && !bAborted && RemainingS <= 3.;
}

bool FRecoveryLaunchSequence::IsGroundSupplyConnected() const
{
    return !bReleaseRequested && (!bRunning || RemainingS > 3.);
}

double FRecoveryLaunchSequence::DelugeDemand() const
{
    // Keep cooling water running after an ignition abort; its valve has physical lag.
    return bRunning && RemainingS <= 10. ? 1. : 0.;
}

const TCHAR* FRecoveryLaunchSequence::Label() const
{
    if (bAborted) return TEXT("LAUNCH ABORT");
    if (bHeld) return TEXT("COUNTDOWN HOLD");
    if (bReleaseRequested) return TEXT("LIFTOFF");
    if (!bRunning) return TEXT("PROPELLANT CONDITIONING");
    if (RemainingS > 30.) return TEXT("TERMINAL COUNT");
    if (RemainingS > 20.) return TEXT("FLIGHT READINESS");
    if (RemainingS > 10.) return TEXT("INTERNAL POWER COMMAND");
    if (RemainingS > 3.) return TEXT("WATER DELUGE");
    return TEXT("ENGINE START / THRUST CHECK");
}

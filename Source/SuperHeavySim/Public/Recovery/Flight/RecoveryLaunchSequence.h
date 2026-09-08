#pragma once

#include "CoreMinimal.h"

/** Final-minute rehearsal. Milestones are declared estimates, not flight certification data.
 * This sequencer commands ground equipment; it never changes a vehicle transform or velocity.
 */
struct SUPERHEAVYSIM_API FRecoveryLaunchSequence
{
    static constexpr double DurationS = 60.;
    double RemainingS = DurationS;
    bool bRunning = false;
    bool bHeld = false;
    bool bAborted = false;
    bool bReleaseRequested = false;

    void Start();
    void Advance(double Dt, bool bSystemsReady, bool bThrustVerified);
    void Abort();
    bool IsIgnitionCommanded() const;
    bool IsGroundSupplyConnected() const;
    double DelugeDemand() const;
    const TCHAR* Label() const;
};

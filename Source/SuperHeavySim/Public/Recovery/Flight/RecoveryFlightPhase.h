#pragma once
#include "CoreMinimal.h"
#include "RecoveryFlightPhase.generated.h"

UENUM(BlueprintType)
enum class ERecoveryPhase : uint8 { Ready, Countdown, Ascent, Separation, Boostback, Coast, Entry, LandingBurn, Capture, Captured, Aborted };

#pragma once

#include "CoreMinimal.h"

/** Presentation-only reconstruction controls. They never change world time or flight state. */
namespace RecoveryRenderSettings
{
    TArray<FString> ReconstructionNames();
    bool SupportsDLSS();
    int32 ApplyReconstruction(int32 RequestedMode);
    FString ActiveReconstruction();
    bool SupportsHardwareRayTracing();
    bool ApplyHardwareRayTracing(bool Requested);
}

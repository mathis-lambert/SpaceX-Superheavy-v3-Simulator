#pragma once
#include "CoreMinimal.h"

/** Immutable CPU counterpart of the render water masks. Safe on the solver thread. */
struct FRecoveryWaterMap
{
    struct FLayer
    {
        uint32 Width=0,Height=0;
        double West=0,South=0,East=0,North=0;
        TArray<uint8> Bits;
    };
    TArray<FLayer> Layers;
    bool IsWater(const FVector& WorldM) const;
    static TSharedPtr<const FRecoveryWaterMap,ESPMode::ThreadSafe> Load();
};

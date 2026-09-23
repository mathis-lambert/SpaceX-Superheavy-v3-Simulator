#pragma once
#include "CoreMinimal.h"

enum class ERecoveryPart : uint8 { None, Engine, Fin, Rcs };
struct FRecoverySelection
{
    ERecoveryPart Kind=ERecoveryPart::None;
    int32 Index=INDEX_NONE;
    bool IsValid() const { return Kind!=ERecoveryPart::None; }
};

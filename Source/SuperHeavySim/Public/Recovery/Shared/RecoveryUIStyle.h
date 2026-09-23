#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
namespace RecoveryUI
{
    extern const FLinearColor Accent, Muted, Ink;
    const FButtonStyle& ButtonStyle();
    const FButtonStyle& PrimaryStyle();
    const FSliderStyle& DaylightSliderStyle();
    const FSliderStyle& ControlSliderStyle();
    const FCheckBoxStyle& ToggleStyle();
}

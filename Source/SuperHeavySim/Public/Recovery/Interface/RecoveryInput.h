#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"

// One owner and one key per action. No mission-reset or scenario hotkeys.
namespace RecoveryInput
{
    enum class EAction : uint8 { Pause, Cameras, NextCamera, FreeCamera, Telemetry, Inspect, Laboratory, Slower, Faster, Start };
    struct FBinding { FKey Key; EAction Action; const TCHAR* Label; };
    inline const TArray<FBinding>& Bindings()
    {
        static const TArray<FBinding> Values={
            {EKeys::Escape,EAction::Pause,TEXT("Pause")},
            {EKeys::Tab,EAction::Cameras,TEXT("Cameras")},
            {EKeys::C,EAction::NextCamera,TEXT("Next camera")},
            {EKeys::F,EAction::FreeCamera,TEXT("Free camera")},
            {EKeys::H,EAction::Telemetry,TEXT("HUD")},
            {EKeys::I,EAction::Inspect,TEXT("Force vectors")},
            {EKeys::L,EAction::Laboratory,TEXT("Flight lab")},
            {EKeys::J,EAction::Slower,TEXT("Slower")},
            {EKeys::K,EAction::Faster,TEXT("Faster")},
            {EKeys::SpaceBar,EAction::Start,TEXT("Start when ready")}};
        return Values;
    }
}

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// No UObject, scene assets or engine fonts: usable on the startup Slate thread.
class RECOVERYLOADING_API SRecoveryLoadingScreen : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryLoadingScreen) {}
        SLATE_ATTRIBUTE(FText,Status)
        SLATE_ATTRIBUTE(TOptional<float>,Progress)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
private:
    FProgressBarStyle BarStyle;
};

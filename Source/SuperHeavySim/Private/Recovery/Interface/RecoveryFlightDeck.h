#pragma once
#include "Widgets/SCompoundWidget.h"
class ARecoveryPlayerController;
class SVerticalBox;

/** Live controls consume the game-thread telemetry snapshot; no physics ownership. */
class SRecoveryFlightDeck : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryFlightDeck) {} SLATE_ARGUMENT(ARecoveryPlayerController*,Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    void Refresh();
    virtual void Tick(const FGeometry& Geometry,double Time,float DeltaTime) override;
private:
    TWeakObjectPtr<ARecoveryPlayerController> Controller;
    TSharedPtr<SVerticalBox> Details;
    TSharedRef<SWidget> Button(const FString& Label,TFunction<void()> Action);
    TSharedRef<SWidget> Text(TAttribute<FText> Value,int32 Size=14);
    int32 Tab=0;
    TArray<FVector> FlownPath;
    uint32 Generation=0;
    double LastSample=-1;
};

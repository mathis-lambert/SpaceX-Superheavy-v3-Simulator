#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameFramework/GameModeBase.h"
#include "SuperHeavyRecoveryHUD.generated.h"
class SWidget;

UCLASS()
class SUPERHEAVYSIM_API ASuperHeavyRecoveryHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TSharedPtr<SWidget> TelemetryWidget;
};

UCLASS()
class SUPERHEAVYSIM_API ASuperHeavyRecoveryGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASuperHeavyRecoveryGameMode();
};

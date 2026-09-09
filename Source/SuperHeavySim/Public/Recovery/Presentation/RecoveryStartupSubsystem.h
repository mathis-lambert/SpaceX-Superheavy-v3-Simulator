#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RecoveryStartupSubsystem.generated.h"
struct FStreamableHandle;
class SRecoveryLoadingScreen;

/** Visible loading follows asset handles, renderer compilation and texture IO. */
UCLASS()
class SUPERHEAVYSIM_API URecoveryStartupSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(URecoveryStartupSubsystem,STATGROUP_Tickables); }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override { return Type==EWorldType::Game || Type==EWorldType::PIE; }
    static bool IsReady(const UWorld* World);
    static bool AssetsLoaded(const UWorld* World);
    bool HasFailed() const { return bFailed; }
private:
    bool bEnabled=false,bAssetsLoaded=false,bReady=false,bFailed=false,bPrecached=false;
    int QuietFrames=0,FrameCount=0,InitialPSOs=0,InitialTextures=0,Loaded=0,Requested=0;
    double Started=0,ReadyAt=0;
    bool bStartupAudit=false,bAuditHomeShot=false,bViewportAttached=false;
    bool bPreviousScreenMessages=true;
    double AuditExitAt=0;
    TSharedPtr<FStreamableHandle> AssetHandle;
    TSharedPtr<SRecoveryLoadingScreen> Screen;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> RetainedAssets;
    FText Status;
    TOptional<float> Progress;
    void WriteReport(int PSOs,int Textures);
};

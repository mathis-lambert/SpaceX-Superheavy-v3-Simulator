#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoveryAudioComponent.generated.h"
class UAudioComponent;

UCLASS(ClassGroup=(Recovery),meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoveryAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryAudioComponent();
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
private:
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> EngineAudio;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> WindAudio;
    bool bInitialized=false;
    float EngineGain=0;
    struct FAcousticSample { double Time,Power;FVector Position; };
    TArray<FAcousticSample> History;
    double HistoryClock=0,LastMissionTime=0;
    void Build();
};

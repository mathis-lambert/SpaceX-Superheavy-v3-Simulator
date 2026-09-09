#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Recovery/Presentation/RecoveryAcoustics.h"
#include "RecoveryAudioComponent.generated.h"
class UAudioComponent;
class USoundBase;

UCLASS(ClassGroup=(Recovery),meta=(BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API URecoveryAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URecoveryAudioComponent();
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn) override;
    bool IsReady() const { return bInitialized && bAssetsValid; }
    double GetHeardPower() const { return HeardPower; }
    double GetHeardDistanceM() const { return HeardDistanceM; }
    double GetDelayS() const { return DelayS; }
    int GetPlayedMechanicalEvents() const { return PlayedMechanicalEvents; }
    int GetHistorySamples() const { return EngineHistory.Num(); }
private:
    enum ELoop:int { Roar,Rumble,Crackle,Wind,Vent,Deluge,Motor,LoopCount };
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Loops;
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> EventVoices;
    TArray<double> EventGains;
    UPROPERTY(Transient) TObjectPtr<USoundBase> ContactSound;
    UPROPERTY(Transient) TObjectPtr<USoundBase> ReleaseSound;
    bool bInitialized=false,bAssetsValid=false,bWasMountReleased=false;
    double EngineGain=0,HeardPower=0,HeardDistanceM=0,DelayS=0,Pitch=1;
    double LastSampleTime=-1,PreviousClosure=0;
    FVector PreviousListener=FVector::ZeroVector;
    int PreviousCamera=-1,ContactMask=0,EventVoice=0,PlayedMechanicalEvents=0;
    uint32 Generation=MAX_uint32;
    RecoveryAcoustics::FHistory EngineHistory,GroundHistory;
    struct FMechanicalEvent { double Time;FVector PositionM;double Strength;bool bRelease; };
    TArray<FMechanicalEvent> Events;
    void Build();
};

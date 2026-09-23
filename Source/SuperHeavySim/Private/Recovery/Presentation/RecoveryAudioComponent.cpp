#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Presentation/RecoveryStartupSubsystem.h"
#include "Recovery/Presentation/RecoveryPropulsionVisuals.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Shared/RecoveryLog.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundBase.h"
#include "Misc/App.h"

URecoveryAudioComponent::URecoveryAudioComponent()
{ PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork; }

void URecoveryAudioComponent::Build()
{
    const TCHAR* Paths[]={RecoveryAssets::S_EngineRoar,RecoveryAssets::S_EngineRumble,RecoveryAssets::S_EngineCrackle,
        RecoveryAssets::S_CoastalWind,RecoveryAssets::S_CryogenicHiss,RecoveryAssets::S_Deluge,RecoveryAssets::S_TowerDrive};
    bAssetsValid=true;
    for(int I=0;I<LoopCount+4;++I)
    {
        auto* Audio=NewObject<UAudioComponent>(GetOwner());
        Audio->bAutoActivate=false;Audio->bAutoDestroy=false;Audio->bAllowSpatialization=I!=Wind && I!=Rumble;
        Audio->bOverrideAttenuation=true;Audio->AttenuationOverrides.bAttenuate=false;
        Audio->AttenuationOverrides.bSpatialize=I!=Wind && I!=Rumble;Audio->SetVolumeMultiplier(0);
        Audio->RegisterComponent();GetOwner()->AddInstanceComponent(Audio);
        if(I<LoopCount)
        {
            auto* Sound=LoadObject<USoundBase>(nullptr,Paths[I]);bAssetsValid&=Sound!=nullptr;
            Audio->SetSound(Sound);Loops.Add(Audio);Audio->Play();
        }
        else {EventVoices.Add(Audio);EventGains.Add(0);}
    }
    ContactSound=LoadObject<USoundBase>(nullptr,RecoveryAssets::S_TowerContact);
    ReleaseSound=LoadObject<USoundBase>(nullptr,RecoveryAssets::S_MountRelease);
    bAssetsValid&=ContactSound && ReleaseSound;
    UE_LOG(LogRecovery,Display,TEXT("RECOVERY_ACOUSTICS ready=%d loops=%d event_voices=%d"),bAssetsValid,Loops.Num(),EventVoices.Num());
    bInitialized=true;
}

void URecoveryAudioComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!URecoveryStartupSubsystem::AssetsLoaded(GetWorld()))return;
    if(!FApp::CanEverRender()){SetComponentTickEnabled(false);return;}
    auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    auto* PC=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    if(!D || !D->GetBody() || !D->Tower || !PC || !PC->GetViewTarget())return;
    if(!bInitialized)Build();
    // World time includes conditioning before T-0 and follows playback.
    const double Now=GetWorld()->GetTimeSeconds();
    const FVector Listener=PC->GetViewTarget()->GetActorLocation()*.01;
    const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody())*.01;
    const FVector Ground=D->Tower->GetCaptureBaseWorld()*.01;
    const double Temperature=D->GetProfile()->SeaLevelTemperatureOffsetK;
    const double Delivered=RecoveryPropulsionVisuals::DeliveredFraction(D->GetEngines(),D->GetProfile()->EngineThrustN);
    if(Generation!=D->GetMissionGeneration())
    {
        EngineHistory.Reset();GroundHistory.Reset();Events.Reset();ContactMask=0;EngineGain=0;
        LastSampleTime=-1;PreviousClosure=D->Tower->ArmClosure;bWasMountReleased=D->IsLaunchMountReleased();
        PreviousCamera=-1;Generation=D->GetMissionGeneration();
        for(const auto& Voice:EventVoices)Voice->Stop();
    }
    const double ArmSpeed=FMath::Abs(D->Tower->ArmClosure-PreviousClosure)/FMath::Max(.001f,Dt);
    PreviousClosure=D->Tower->ArmClosure;
    if(LastSampleTime<0 || Now-LastSampleTime>=.045)
    {
        EngineHistory.Add({Now,Base,D->VelocityMps,FVector4(Delivered*RecoveryAcoustics::Air(D->AltitudeM),0,0,0)});
        GroundHistory.Add({Now,Ground,FVector::ZeroVector,FVector4(0,D->GetConditioningFlowKgS().Size(),D->GetDelugeFlow(),FMath::Clamp(ArmSpeed*5,0.,1.))});
        LastSampleTime=Now;
    }
    if(D->IsLaunchMountReleased() && !bWasMountReleased)Events.Add({Now,Ground,.6,true});
    bWasMountReleased=D->IsLaunchMountReleased();
    // Once per verified rail: persistent contact must not chatter.
    for(int Side=0;Side<2;++Side)
        if(D->SupportImpulseNs[Side]>0 && !(ContactMask&(1<<Side)))
        {
            ContactMask|=1<<Side;
            const UBoxComponent* Rail=Side==0?D->Tower->LeftRail.Get():D->Tower->RightRail.Get();
            Events.Add({Now,Rail->GetComponentLocation()*.01,FMath::Clamp(FMath::Sqrt(D->SupportImpulseNs[Side]/150000.),.12,1.),false});
        }
    const auto Heard=EngineHistory.Hear(Now,Listener,Temperature);
    const auto Site=GroundHistory.Hear(Now,Listener,Temperature);
    const double Air=RecoveryAcoustics::Air(Listener.Z);
    HeardPower=Heard.Channels.X*Air;HeardDistanceM=FVector::Distance(Heard.PositionM,Listener);
    DelayS=FMath::Max(0.,Now-Heard.Time);
    const double Gain=RecoveryAcoustics::Gain(HeardDistanceM,HeardPower);
    // A camera cut is not a physical observer moving thousands of km/s.
    const FVector ObserverVelocity=PreviousCamera==D->Viewer->GetCameraMode() && FVector::Distance(Listener,PreviousListener)<2000?
        (Listener-PreviousListener)/FMath::Max(.001f,Dt):FVector::ZeroVector;
    PreviousListener=Listener;PreviousCamera=D->Viewer->GetCameraMode();
    const double TargetPitch=RecoveryAcoustics::Doppler(Heard.VelocityMps,ObserverVelocity,(Listener-Heard.PositionM).GetSafeNormal());
    Pitch=RecoveryAcoustics::Smooth(Pitch,TargetPitch,Dt,.2,.2);
    EngineGain=RecoveryAcoustics::Smooth(EngineGain,Gain,Dt);
    const double Master=URecoveryStartupSubsystem::IsReady(GetWorld())?FMath::Clamp(double(PC->MasterVolume),0.,1.):0;
    const double Near=1-FMath::SmoothStep(300.,2200.,HeardDistanceM);
    const double Structural=D->Viewer->GetCameraMode()==5?FMath::Sqrt(Delivered)*.10:0;
    const double GroundDistance=FVector::Distance(Ground,Listener);
    const double Values[]={EngineGain*.48,EngineGain*.30+Structural,EngineGain*.13*Near,.12*Air,
        FMath::Sqrt(FMath::Clamp(Site.Channels.Y,0.,1.))*.16/(1+GroundDistance/80.)*Air,
        Site.Channels.Z*.23/(1+GroundDistance/250.)*Air,Site.Channels.W*.15/(1+GroundDistance/90.)*Air};
    double Sum=0;for(double Value:Values)Sum+=Value;
    const double Headroom=.70/FMath::Max(1.,Sum);
    for(int I=0;I<LoopCount;++I)
    {
        auto* A=Loops[I].Get();A->SetVolumeMultiplier(Values[I]*Master*Headroom);
        A->SetWorldLocation((I<Wind?Heard.PositionM:Ground)*100);
        if(I<Wind)
        {
            A->SetPitchMultiplier(Pitch*(I==Rumble?.93:1.));
            A->SetLowPassFilterEnabled(true);
            A->SetLowPassFilterFrequency(FMath::Clamp(15000./(1+HeardDistanceM/650.),450.,15000.));
        }
    }
    for(int I=0;I<EventVoices.Num();++I)
        EventVoices[I]->SetVolumeMultiplier(Master*Air*EventGains[I]/(1+FVector::Distance(EventVoices[I]->GetComponentLocation()*.01,Listener)/150.));
    for(int I=Events.Num()-1;I>=0;--I)
    {
        const auto& E=Events[I];
        const double Arrival=E.Time+RecoveryAcoustics::TravelSeconds(E.PositionM,Listener,Temperature);
        if(Now<Arrival)continue;
        if(Now-Arrival<1.5)
        {
            const int Slot=EventVoice++%EventVoices.Num();EventGains[Slot]=.20*E.Strength;
            auto* Voice=EventVoices[Slot].Get();Voice->Stop();
            Voice->SetSound(E.bRelease?ReleaseSound:ContactSound);Voice->SetWorldLocation(E.PositionM*100);
            Voice->SetVolumeMultiplier(Master*Air*.20*E.Strength/(1+FVector::Distance(E.PositionM,Listener)/150.));
            Voice->SetLowPassFilterEnabled(true);Voice->SetLowPassFilterFrequency(FMath::Clamp(10000./(1+FVector::Distance(E.PositionM,Listener)/400.),600.,10000.));
            Voice->Play(Now-Arrival);++PlayedMechanicalEvents;
            UE_LOG(LogRecovery,Display,TEXT("RECOVERY_MECHANICAL event=%s delay_s=%.3f strength=%.3f"),E.bRelease?TEXT("mount_release"):TEXT("rail_contact"),Arrival-E.Time,E.Strength);
        }
        Events.RemoveAt(I);
    }
}

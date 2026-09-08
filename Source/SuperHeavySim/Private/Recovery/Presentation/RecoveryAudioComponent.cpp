#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Misc/App.h"

URecoveryAudioComponent::URecoveryAudioComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}

void URecoveryAudioComponent::Build()
{
    const TCHAR* Paths[]={RecoveryAssets::S_EngineRoar,RecoveryAssets::S_CoastalWind};
    for(int I=0;I<2;++I)
    {
        auto* Sound=LoadObject<USoundWave>(nullptr,Paths[I]);if(!Sound)continue;
        auto* Audio=NewObject<UAudioComponent>(GetOwner());
        Audio->SetAutoActivate(false);Audio->bAutoDestroy=false;
        Audio->SetSound(Sound);Audio->SetVolumeMultiplier(0);
        Audio->bAllowSpatialization=I==0;
        Audio->bOverrideAttenuation=true;Audio->AttenuationOverrides.bAttenuate=false;
        Audio->AttenuationOverrides.bSpatialize=I==0;
        Audio->RegisterComponent();GetOwner()->AddInstanceComponent(Audio);Audio->Play();
        if(I==0)EngineAudio=Audio;else WindAudio=Audio;
    }
    bInitialized=true;
}

void URecoveryAudioComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!FApp::CanEverRender()){SetComponentTickEnabled(false);return;}
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    const auto* PC=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    if(!D || !D->GetBody() || !PC || !PC->GetViewTarget())return;
    if(!bInitialized)Build();
    const FVector Listener=PC->GetViewTarget()->GetActorLocation();
    const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody());
    if(D->MissionTime<LastMissionTime){History.Reset();EngineGain=0;}
    LastMissionTime=D->MissionTime;
    HistoryClock+=Dt;
    if(HistoryClock>.05)
    {
        HistoryClock=0;
        History.Add({D->MissionTime,D->Throttle*FMath::Sqrt(D->ActiveEngines/33.)*FMath::Sqrt(FMath::Clamp(D->PressurePa/101325.,0.,1.)),Base});
        if(History.Num()>1200)History.RemoveAt(0,History.Num()-1200,EAllowShrinking::No);
    }
    double Power=0;FVector Source=Base;
    // Listen to the latest source event whose propagation has reached the camera.
    // The 60-second history covers every supplied ground observer camera.
    for(int I=History.Num()-1;I>=0;--I)
        if(History[I].Time+(History[I].Position-Listener).Size()/34300.<=D->MissionTime)
        {Power=History[I].Power;Source=History[I].Position;break;}
    const double DistanceM=(Source-Listener).Size()/100;
    const double Air=1-FMath::SmoothStep(8000.,45000.,FlightGeometry::AltitudeM(Listener));
    const float Gain=PC->MasterVolume*.8*Power*Air/FMath::Pow(1+DistanceM/300.,.65);
    EngineGain=FMath::FInterpTo(EngineGain,Gain,Dt,4.);
    if(EngineAudio)
    {
        EngineAudio->SetWorldLocation(Source);EngineAudio->SetVolumeMultiplier(EngineGain);
        EngineAudio->SetLowPassFilterEnabled(true);
        EngineAudio->SetLowPassFilterFrequency(FMath::Clamp(12000./(1+DistanceM/800),550.,12000.));
        EngineAudio->SetPitchMultiplier(.9f+.14f*Power);
    }
    if(WindAudio)WindAudio->SetVolumeMultiplier(PC->MasterVolume*.22*Air);
}

"""One-time, checked source transformation; original files are in ExperienceBaseline."""
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
p=ROOT/'Source/SuperHeavySim/Private/Recovery/RecoveryPresentationComponent.cpp'
s=p.read_text(encoding='utf-8')
start=s.index('    auto* Plane=LoadObject<UStaticMesh>')
end=s.index('    for(int I=0;I<6;++I)',start)
s=s[:start]+s[end:]
start=s.index('    if(D->Phase==ERecoveryPhase::Ready || D->Phase==ERecoveryPhase::Countdown)')
s=s[:start]+'    D->UpdateCamera(Dt);\n}\n'
s=s.replace('#include "Recovery/Interface/RecoveryPlayerController.h"','#include "Recovery/Interface/RecoveryPlayerController.h"\n#include "Recovery/Shared/FlightGeometry.h"')
s=s.replace('Light->SetAttenuationRadius(12000);Light->SetSourceRadius(65);Light->SetSoftSourceRadius(120);','Light->SetAttenuationRadius(1800);Light->SetSourceRadius(35);Light->SetSoftSourceRadius(65);')
s=s.replace('Light->SetCastShadows(C->GetName().StartsWith(TEXT("RGC")));','Light->SetCastShadows(false);')
s=s.replace('Light->SetVolumetricScatteringIntensity(0.12f);','Light->SetVolumetricScatteringIntensity(0.f);')
s=s.replace('3000000.*D->Throttle*Flicker*LightScale','25000.*D->Throttle*Flicker*LightScale')
s=s.replace('const FVector Base=D->GetBody()->GetComponentLocation()-Up*3544;','const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody());')
s=s.replace('const FVector Positions[]={FVector(4.55,0,62),FVector(4.55,0,12),FVector(-4.55,0,62),FVector(-4.55,0,12),FVector(0,4.55,52),FVector(0,-4.55,52)};','const auto& Positions=FlightGeometry::ReactionNozzlePositionsM();')
s=s.replace('    UpperStage=NewObject<UStaticMeshComponent>(GetOwner());','''    // Broad, distributed light along the actual emitting column. Only the upper
    // source casts shadows; the small nozzle lights illuminate local metal only.
    for(int I=0;I<3;++I)
    {
        auto* Light=NewObject<UPointLightComponent>(GetOwner());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Candelas);
        Light->SetAttenuationRadius(30000);
        Light->SetSourceRadius(500);Light->SetSoftSourceRadius(900);
        Light->SetCastShadows(I==0);
        Light->SetVolumetricScatteringIntensity(1.f);
        Light->SetLightColor(FLinearColor(1.f,.68f,.38f));
        Light->SetIntensity(0);Light->SetVisibility(false);
        Light->RegisterComponent();GetOwner()->AddInstanceComponent(Light);PlumeLights.Add(Light);
    }
    UpperStage=NewObject<UStaticMeshComponent>(GetOwner());''')
s=s.replace('    MixingPlume->SetWorldLocationAndRotation(Base-Up*900,D->GetBody()->GetComponentQuat());','''    for(int I=0;I<PlumeLights.Num();++I)
    {
        PlumeLights[I]->SetWorldLocation(Base-Up*(1000+I*1500));
        PlumeLights[I]->SetIntensity(1200000.*(D->ActiveEngines/13.)*FMath::Pow(D->Throttle,1.5)*LightScale/(1+I*.5));
        PlumeLights[I]->SetVisibility(D->ActiveEngines>0 && D->Throttle>.01 && LightScale>0);
    }
    MixingPlume->SetWorldLocationAndRotation(Base-Up*900,D->GetBody()->GetComponentQuat());''')
s=s.replace('// Each source follows its own gimballed nozzle and engine group. Candela\n        // makes the lighting visible in the scene\'s daylight exposure.','// Small sources reveal the nozzle rim without lighting the entire site 33 times.')
s=s.replace('// Three central sources retain contact shadows without 33 shadow maps.','// The distributed plume provides the scene shadow; nozzle sources stay local.')
p.write_text(s,encoding='utf-8')
p=ROOT/'Source/SuperHeavySim/Public/Recovery/Presentation/RecoveryPresentationComponent.h'
s=p.read_text(encoding='utf-8')
start=s.index('    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Puffs;')
end=s.index('    FVector ShipPosition',start)
s=s[:start]+'    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> PlumeLights;\n'+s[end:]
s=s.replace('double Clock=0,SpawnClock=0;','double Clock=0;').replace('int PuffIndex=0,LastLitEngineCount=-1;','int LastLitEngineCount=-1;')
p.write_text(s,encoding='utf-8')
p=ROOT/'Source/SuperHeavySim/Private/Recovery/SuperHeavyRecoveryDirector.cpp'
s=p.read_text(encoding='utf-8').replace('#include "Recovery/Presentation/RecoverySkyComponent.h"','#include "Recovery/Presentation/RecoverySkyComponent.h"\n#include "Recovery/Presentation/RecoveryVaporComponent.h"')
s=s.replace('    CreateDefaultSubobject<URecoverySkyComponent>(TEXT("SkyAndCamera"));','    CreateDefaultSubobject<URecoverySkyComponent>(TEXT("SkyAndCamera"));\n    CreateDefaultSubobject<URecoveryVaporComponent>(TEXT("ParticipatingVapor"));')
p.write_text(s,encoding='utf-8')
print('EXPERIENCE_SOURCE_MIGRATION_READY')

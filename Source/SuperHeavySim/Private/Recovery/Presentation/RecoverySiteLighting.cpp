#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Presentation/RecoverySkyComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

void URecoverySkyComponent::BuildSiteLighting()
{
    auto* FixtureMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* FixtureMaterial=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_SiteLamp);
    auto* HousingMaterial=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_Graphite);
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    const auto* Tower=D?D->Tower.Get():nullptr;
    const float Height=Tower?Tower->TowerHeightM*100:10500;
    const FTransform Site=Tower?Tower->GetActorTransform():FTransform::Identity;
    const auto MakePart=[&](const FVector& LocalPosition,const FVector& Scale,UMaterialInterface* Material,const FRotator& Rotation=FRotator::ZeroRotator)
    {
        if(!FixtureMesh || !Material)return;
        auto* Part=NewObject<UStaticMeshComponent>(GetOwner());
        Part->SetStaticMesh(FixtureMesh);Part->SetMaterial(0,Material);
        Part->SetWorldLocationAndRotation(Site.TransformPosition(LocalPosition),Site.TransformRotation(Rotation.Quaternion()));
        Part->SetWorldScale3D(Scale);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(false);Part->RegisterComponent();GetOwner()->AddInstanceComponent(Part);
    };
    // Real fixtures surround the mount and illuminate the tower from several heights.
    const FVector Locations[]={
        {5500,-2800,1200},{5500,2800,1200},{-1800,-2500,1100},{-1800,2500,1100},
        {-600,-600,Height*.30f},{-600,600,Height*.50f},{-600,-600,Height*.72f},{-600,600,Height*.92f},
        {13500,3900,650},{17900,3900,650},{11300,-5500,520},{-11700,8200,900}};
    for(int32 I=0;I<UE_ARRAY_COUNT(Locations);++I)
    {
        const FVector Focus=I<4?FVector(2400,0,I<2?8500:900):I<8?FVector(1100,0,Locations[I].Z-1800):Locations[I]+FVector(0,-1600,-Locations[I].Z);
        auto* Light=NewObject<USpotLightComponent>(GetOwner());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetWorldLocationAndRotation(Site.TransformPosition(Locations[I]),Site.TransformVector(Focus-Locations[I]).Rotation());
        Light->SetIntensityUnits(ELightUnits::Candelas);Light->SetIntensity(0);
        Light->SetAttenuationRadius(I<8?16000:4500);Light->SetInnerConeAngle(35);Light->SetOuterConeAngle(65);
        Light->SetSourceRadius(35);Light->SetLightColor(FLinearColor(1.f,.86f,.68f));
        if(I>=8){Light->SetTemperature(4700);Light->SetUseTemperature(true);Light->MaxDrawDistance=120000;Light->MaxDistanceFadeRange=20000;}
        Light->SetCastShadows(I<2);Light->SetVolumetricScatteringIntensity(.6f);
        Light->RegisterComponent();GetOwner()->AddInstanceComponent(Light);FloodLights.Add(Light);
        MakePart(Locations[I],FVector(.15,.65,.32),FixtureMaterial,(Focus-Locations[I]).Rotation());
        if(I<4)MakePart(FVector(Locations[I].X,Locations[I].Y,Locations[I].Z*.5),FVector(.18,.18,Locations[I].Z/100),HousingMaterial);
    }
    for(int32 I=0;I<SiteLights.Num();++I)
    {
        const FVector Position(-9000+(I%6)*4000,I<6?-10500:10000,600);
        SiteLights[I]->SetWorldLocation(Site.TransformPosition(Position));
        MakePart(FVector(Position.X,Position.Y,300),FVector(.12,.12,6),HousingMaterial);
        MakePart(Position,FVector(.35,.45,.1),FixtureMaterial);
    }
    auto* BeaconMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    for(const FVector Position:{FVector(0,-450,Height+70),FVector(0,450,Height+70),FVector(-600,0,Height*.55)})
    {
        if(!FixtureMaterial)continue;
        auto* Material=UMaterialInstanceDynamic::Create(FixtureMaterial,this);
        Material->SetVectorParameterValue(TEXT("Color"),FLinearColor(1.f,.025f,.012f));
        auto* Beacon=NewObject<UStaticMeshComponent>(GetOwner());
        Beacon->SetStaticMesh(BeaconMesh);Beacon->SetMaterial(0,Material);
        Beacon->SetWorldLocation(Site.TransformPosition(Position));Beacon->SetWorldScale3D(FVector(.25));
        Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);Beacon->SetCastShadow(false);
        Beacon->RegisterComponent();GetOwner()->AddInstanceComponent(Beacon);BeaconMaterials.Add(Material);
    }
}

void URecoverySkyComponent::UpdateSiteLighting(float Night)
{
    for(const auto& Light:SiteLights)Light->SetIntensity(3000*Night);
    for(int32 I=0;I<FloodLights.Num();++I)FloodLights[I]->SetIntensity((I<2?180000:22000)*Night);
    const double Time=GetWorld()->GetRealTimeSeconds();
    for(int32 I=0;I<BeaconMaterials.Num();++I)
    {
        const double Pulse=FMath::Fmod(Time*.8+I*.08,1.);
        BeaconMaterials[I]->SetScalarParameterValue(TEXT("Intensity"),Pulse<.15?1600.f:2.f);
    }
}

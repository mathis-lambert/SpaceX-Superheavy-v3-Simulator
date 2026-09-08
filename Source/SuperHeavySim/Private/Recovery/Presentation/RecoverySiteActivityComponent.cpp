#include "Recovery/Presentation/RecoverySiteActivityComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"

URecoverySiteActivityComponent::URecoverySiteActivityComponent()
{PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostPhysics;}

void URecoverySiteActivityComponent::Build(const FTransform& Site)
{
    auto* TruckMesh=LoadObject<UStaticMesh>(nullptr,RecoveryAssets::SM_ServicePickup);
    auto* FlagMesh=LoadObject<UStaticMesh>(nullptr,RecoveryAssets::SM_WindFlag);
    auto* Cloth=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_WindFlag);
    auto* Metal=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_Cladding);
    auto* Lamp=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_SiteLamp);
    const auto Part=[&](UStaticMesh* Mesh,UMaterialInterface* Mat,FVector Location,FVector Scale)
    {
        auto* C=NewObject<UStaticMeshComponent>(GetOwner());C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(Mesh);if(Mat)C->SetMaterial(0,Mat);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetGenerateOverlapEvents(false);C->SetCullDistance(180000);C->RegisterComponent();GetOwner()->AddInstanceComponent(C);
        C->SetWorldLocation(Site.TransformPosition(Location*100));C->SetWorldScale3D(Scale);return C;
    };
    if(TruckMesh)for(int I=0;I<2;++I)
    {
        auto* Truck=Part(TruckMesh,nullptr,FVector(-255+I*55,-93.5,0),FVector(1,-1,1));Trucks.Add(Truck);
        if(Lamp)
        {
            auto* M=UMaterialInstanceDynamic::Create(Lamp,this);M->SetVectorParameterValue(TEXT("Color"),FLinearColor(1,.35f,.025f));
            auto* B=Part(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")),M,FVector::ZeroVector,FVector(.15,.15,.12));
            B->AttachToComponent(Truck,FAttachmentTransformRules::KeepRelativeTransform);B->SetRelativeLocation(FVector(12,0,252));B->SetCastShadow(false);Beacons.Add(M);
            auto* Lens=UMaterialInstanceDynamic::Create(Lamp,this);
            Lens->SetVectorParameterValue(TEXT("Color"),FLinearColor(1,.94f,.83f));Lens->SetScalarParameterValue(TEXT("Intensity"),25.f);
            for(double Side:{-74.,74.})
            {
                auto* Glass=Part(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")),Lens,FVector::ZeroVector,FVector(.035,.4,.2));
                Glass->AttachToComponent(Truck,FAttachmentTransformRules::KeepRelativeTransform);Glass->SetRelativeLocation(FVector(275,Side,120));Glass->SetCastShadow(false);
            }
        }
        // One bounded beam per vehicle covers both lenses. Service headlights
        // stay on in daylight; no extra shadow map or launch-site-wide light.
        auto* Beam=NewObject<USpotLightComponent>(GetOwner());Beam->SetMobility(EComponentMobility::Movable);
        Beam->SetupAttachment(Truck);Beam->SetRelativeLocation(FVector(280,0,120));Beam->SetRelativeRotation(FRotator(-6,0,0));
        Beam->SetIntensityUnits(ELightUnits::Candelas);Beam->SetIntensity(16000);
        Beam->SetAttenuationRadius(6000);Beam->SetInnerConeAngle(16);Beam->SetOuterConeAngle(30);Beam->SetSourceRadius(35);
        Beam->SetLightColor(FLinearColor(1,.94f,.83f));Beam->SetCastShadows(false);Beam->SetVolumetricScatteringIntensity(.15f);
        Beam->MaxDrawDistance=70000;Beam->MaxDistanceFadeRange=10000;
        Beam->RegisterComponent();GetOwner()->AddInstanceComponent(Beam);
    }
    if(FlagMesh && Cloth)for(int I=0;I<3;++I)
    {
        const FVector Base(-145+I*6,-118,0);
        Part(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")),Metal,Base+FVector(0,0,4),FVector(.08,.08,8));
        auto* M=UMaterialInstanceDynamic::Create(Cloth,this);
        Flags.Add(Part(FlagMesh,M,Base+FVector(0,0,6.7),FVector(1,-1,1)));ClothMaterials.Add(M);
    }
    bBuilt=true;
}
void URecoverySiteActivityComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!FApp::CanEverRender()){SetComponentTickEnabled(false);return;}
    auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());if(!D || !D->Tower)return;
    const auto Site=D->Tower->GetActorTransform();if(!bBuilt)Build(Site);
    ActivityTime+=Dt;
    // Keep the access road static throughout an active launch/catch operation.
    // These generic service vehicles patrol only the western service section.
    if(D->Phase==ERecoveryPhase::Ready)TrafficDistance+=Dt*1.8;
    constexpr double Length=65,Radius=4.7,Loop=2*Length+2*PI*Radius;
    for(int I=0;I<Trucks.Num();++I)
    {
        const double T=FMath::Fmod(TrafficDistance+I*Loop*.5,Loop);FVector P;double Yaw;
        if(T<Length){P=FVector(-270+T,-90-Radius,.1);Yaw=0;}
        else if(T<Length+PI*Radius){double A=(T-Length)/Radius-PI*.5;P=FVector(-205+Radius*FMath::Cos(A),-90+Radius*FMath::Sin(A),.1);Yaw=FMath::RadiansToDegrees(A+PI*.5);}
        else if(T<2*Length+PI*Radius){P=FVector(-205-(T-Length-PI*Radius),-90+Radius,.1);Yaw=180;}
        else{double A=(T-2*Length-PI*Radius)/Radius+PI*.5;P=FVector(-270+Radius*FMath::Cos(A),-90+Radius*FMath::Sin(A),.1);Yaw=FMath::RadiansToDegrees(A+PI*.5);}
        Trucks[I]->SetWorldLocationAndRotation(Site.TransformPosition(P*100),Site.TransformRotation(FRotator(0,Yaw,0).Quaternion()));
        if(Beacons.IsValidIndex(I))Beacons[I]->SetScalarParameterValue(TEXT("Intensity"),FMath::Fmod(ActivityTime+I*.3,1)<.12?180.f:1.f);
    }
    const FVector Wind=D->GetWindVelocityMps(10);const double Yaw=Wind.IsNearlyZero()?0:Wind.Rotation().Yaw;
    for(int I=0;I<Flags.Num();++I)
    {
        Flags[I]->SetWorldRotation(FRotator(0,Yaw,0));const FVector Normal=Flags[I]->GetRightVector();
        ClothMaterials[I]->SetScalarParameterValue(TEXT("ActivityTime"),ActivityTime+I*.43);
        ClothMaterials[I]->SetScalarParameterValue(TEXT("WindSpeed"),Wind.Size());
        ClothMaterials[I]->SetVectorParameterValue(TEXT("ClothNormal"),FLinearColor(Normal.X,Normal.Y,Normal.Z));
    }
}

#include "Recovery/Presentation/RecoveryStartupSubsystem.h"
#include "Recovery/Presentation/RecoverySiteActivityComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Presentation/RecoveryEnvironmentProfile.h"
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
    if(const auto* Profile=LoadObject<URecoveryEnvironmentProfile>(nullptr,RecoveryAssets::DA_RecoveryEnvironment))Road=Profile->ServiceRoad;
    if(TruckMesh)for(int I=0;I<4;++I)
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
    if(!URecoveryStartupSubsystem::AssetsLoaded(GetWorld()))return;
    if(!FApp::CanEverRender()){SetComponentTickEnabled(false);return;}
    auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());if(!D || !D->Tower)return;
    const auto Site=D->Tower->GetActorTransform();if(!bBuilt)Build(Site);
    ActivityTime+=Dt;
    // Shared road data prevents vehicle paths drifting away from the authored
    // mesh. Clear traffic at countdown; the viewing effects never drive physics.
    const bool TrafficAllowed=D->Phase==ERecoveryPhase::Ready && !D->GetLaunchSequence().bRunning;
    const double Duty=FMath::Fmod(ActivityTime,85.);
    TrafficSpeed=FMath::FInterpConstantTo(TrafficSpeed,TrafficAllowed && Duty<72?3.2:0.,Dt,1.6);
    TrafficDistance+=Dt*TrafficSpeed;
    double Loop=0;for(int I=1;I<Road.Num();++I)Loop+=FVector::Dist(Road[I-1],Road[I]);
    for(int I=0;I<Trucks.Num();++I)
    {
        FVector P(136+(I-2)*24,26,.07);double Yaw=90;
        if(I<2 && Loop>0)
        {
            double Distance=FMath::Fmod(TrafficDistance+I*Loop*.48,Loop);
            for(int Segment=1;Segment<Road.Num();++Segment)
            {
                const FVector Delta=Road[Segment]-Road[Segment-1];const double Length=Delta.Size();
                if(Length<=KINDA_SMALL_NUMBER)continue;
                if(Distance<=Length){P=Road[Segment-1]+Delta*(Distance/Length);Yaw=Delta.Rotation().Yaw;break;}
                Distance-=Length;
            }
        }
        const FRotator Target=Site.TransformRotation(FRotator(0,Yaw,0).Quaternion()).Rotator();
        Trucks[I]->SetWorldLocationAndRotation(Site.TransformPosition(P*100),FMath::RInterpTo(Trucks[I]->GetComponentRotation(),Target,Dt,5.));
        if(Beacons.IsValidIndex(I))Beacons[I]->SetScalarParameterValue(TEXT("Intensity"),FMath::Fmod(ActivityTime+I*.3,1)<.12?180.f:1.f);
    }
    const FVector Wind=D->GetWindVelocityMps(10);const double Yaw=Wind.IsNearlyZero()?0:Wind.Rotation().Yaw;
    UpdateFacilityVents(Site,Wind);
    for(int I=0;I<Flags.Num();++I)
    {
        Flags[I]->SetWorldRotation(FRotator(0,Yaw,0));const FVector Normal=Flags[I]->GetRightVector();
        ClothMaterials[I]->SetScalarParameterValue(TEXT("ActivityTime"),ActivityTime+I*.43);
        ClothMaterials[I]->SetScalarParameterValue(TEXT("WindSpeed"),Wind.Size());
        ClothMaterials[I]->SetVectorParameterValue(TEXT("ClothNormal"),FLinearColor(Normal.X,Normal.Y,Normal.Z));
    }
}

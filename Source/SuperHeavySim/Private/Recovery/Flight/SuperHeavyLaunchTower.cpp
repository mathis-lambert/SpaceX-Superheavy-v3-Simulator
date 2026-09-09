#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Flight/RecoveryRailSupport.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"

ASuperHeavyLaunchTower::ASuperHeavyLaunchTower()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SiteOrigin"));
    Structure = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TowerStructure"));
    Structure->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Structure->SetStaticMesh(Cube.Object);
    Structure->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ArchitecturalDetails = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ServiceCore"));
    ArchitecturalDetails->SetupAttachment(RootComponent);
    ArchitecturalDetails->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Carriage = CreateDefaultSubobject<USceneComponent>(TEXT("CaptureCarriage"));
    Carriage->SetupAttachment(RootComponent);
    LeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftCaptureArm"));
    RightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightCaptureArm"));
    for (auto* Arm : {LeftArm.Get(), RightArm.Get()})
    { Arm->SetupAttachment(Carriage); Arm->SetStaticMesh(Cube.Object); Arm->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
    CaptureConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("CaptureRestraint"));
    CaptureConstraint->SetupAttachment(RootComponent);
    // Retained only for old Blueprint serialization. It is never connected.
    LeftRail=CreateDefaultSubobject<UBoxComponent>(TEXT("LeftLoadBearingRail"));
    RightRail=CreateDefaultSubobject<UBoxComponent>(TEXT("RightLoadBearingRail"));
    LeftArmCollider=CreateDefaultSubobject<UBoxComponent>(TEXT("LeftArmContactVolume"));
    RightArmCollider=CreateDefaultSubobject<UBoxComponent>(TEXT("RightArmContactVolume"));
    for(int32 Side=0;Side<2;++Side)
    {
        auto* Arm=Side==0?LeftArm.Get():RightArm.Get();
        for(auto* Box : {Side==0?LeftRail.Get():RightRail.Get(),Side==0?LeftArmCollider.Get():RightArmCollider.Get()})
        {
            Box->SetupAttachment(Arm);Box->SetCollisionProfileName(TEXT("BlockAll"));
            Box->SetNotifyRigidBodyCollision(true);Box->SetGenerateOverlapEvents(false);
            Box->SetRelativeScale3D(FVector(1./26,1./1.1,1./1.7));
        }
        auto* Rail=Side==0?LeftRail.Get():RightRail.Get();
        Rail->SetBoxExtent(FVector(RecoveryContactGeometry::RailHalfLengthM*100,42.5,9));Rail->SetRelativeLocation(FVector(0,0,96./1.7));
        auto* Frame=Side==0?LeftArmCollider.Get():RightArmCollider.Get();
        Frame->SetBoxExtent(FVector(1300,55,85));
    }
}

void ASuperHeavyLaunchTower::Beam(const FVector& A, const FVector& B, double Width)
{
    const FVector Delta = B - A;
    Structure->AddInstance(FTransform(FRotationMatrix::MakeFromZ(Delta).ToQuat(), (A+B)*50, FVector(Width,Width,Delta.Length())));
}

void ASuperHeavyLaunchTower::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Structure->ClearInstances();
    const double H = FMath::Clamp(TowerHeightM, 85., 140.);
    ArchitecturalDetails->SetRelativeScale3D(FVector(1,1,H/105.));
    for (int X : {-1,1}) for (int Y : {-1,1}) Beam(FVector(X*6,Y*6,0),FVector(X*6,Y*6,H),0.9);
    const int Floors = FMath::FloorToInt(H/10);
    for (int I=0; I<=Floors; ++I)
    {
        const double Z = I*10;
        for(int S : {-1,1})
        {
            Beam(FVector(-6,S*6,Z),FVector(6,S*6,Z),0.55);
            Beam(FVector(S*6,-6,Z),FVector(S*6,6,Z),0.55);
            if(I<Floors)
            {
                Beam(FVector(-6,S*6,Z),FVector(6,S*6,Z+10),0.3);
                Beam(FVector(6,S*6,Z),FVector(-6,S*6,Z+10),0.3);
                Beam(FVector(S*6,-6,Z),FVector(S*6,6,Z+10),0.3);
                Beam(FVector(S*6,6,Z),FVector(S*6,-6,Z+10),0.3);
            }
        }
    }
    // Twin carriage rails and top gantry.
    for(int Y : {-1,1}) Beam(FVector(7,Y*4,4),FVector(7,Y*4,H),0.35);
    Beam(FVector(-7,-7,H),FVector(12,-7,H),1);
    Beam(FVector(-7,7,H),FVector(12,7,H),1);
    Carriage->SetRelativeLocation(FVector(0,0,(CaptureOffsetM.Z+ArmContactHeightAboveBaseM)*100));
    for(auto* Arm : {LeftArm.Get(),RightArm.Get()}) Arm->SetRelativeScale3D(FVector(26,1.1,1.7));
    SetArmClosure(ArmClosure);
}

FVector ASuperHeavyLaunchTower::GetCaptureBaseWorld() const
{ return GetActorTransform().TransformPosition(CaptureOffsetM*100); }

void ASuperHeavyLaunchTower::SetArmClosure(double Value)
{
    ArmClosure=FMath::Clamp(Value,0.,1.);
    const double Gap=FMath::Lerp(10.,5.15,ArmClosure);
    LeftArm->SetRelativeLocation(FVector((CaptureOffsetM.X+RecoveryContactGeometry::RailCentreOffsetM)*100,(CaptureOffsetM.Y-Gap)*100,0));
    RightArm->SetRelativeLocation(FVector((CaptureOffsetM.X+RecoveryContactGeometry::RailCentreOffsetM)*100,(CaptureOffsetM.Y+Gap)*100,0));
    LeftArm->SetRelativeRotation(FRotator(0,FMath::Lerp(-12.,0.,ArmClosure),0));
    RightArm->SetRelativeRotation(FRotator(0,FMath::Lerp(12.,0.,ArmClosure),0));
}

bool ASuperHeavyLaunchTower::IsSupport(const UPrimitiveComponent* Component,int32& Side) const
{
    if(Component==LeftRail) { Side=0;return true; }
    if(Component==RightRail) { Side=1;return true; }
    return false;
}

void ASuperHeavyLaunchTower::Release() { CaptureConstraint->BreakConstraint(); SetArmClosure(0); }

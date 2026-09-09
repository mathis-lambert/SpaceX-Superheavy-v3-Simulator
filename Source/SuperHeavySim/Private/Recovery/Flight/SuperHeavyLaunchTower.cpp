#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Flight/RecoveryRailSupport.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"

ASuperHeavyLaunchTower::ASuperHeavyLaunchTower()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PostPhysics;
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
    LeftHinge=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LeftMotorHinge"));
    RightHinge=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("RightMotorHinge"));
    LeftSuspension=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LeftRailSuspension"));
    RightSuspension=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("RightRailSuspension"));
    LeftRailVisual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftMovingRail"));
    RightRailVisual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightMovingRail"));
    for(int32 Side=0;Side<2;++Side)
    {
        for(auto* Box : {Side==0?LeftRail.Get():RightRail.Get(),Side==0?LeftArmCollider.Get():RightArmCollider.Get()})
        {
            Box->SetupAttachment(Carriage);Box->SetCollisionProfileName(TEXT("PhysicsActor"));
            Box->SetNotifyRigidBodyCollision(true);Box->SetGenerateOverlapEvents(false);
            Box->BodyInstance.bAutoWeld=false;Box->BodyInstance.bUseCCD=true;
            Box->BodyInstance.SetPositionSolverIterationCount(24);Box->BodyInstance.SetVelocitySolverIterationCount(12);
        }
        auto* Rail=Side==0?LeftRail.Get():RightRail.Get();
        Rail->SetBoxExtent(FVector(RecoveryContactGeometry::RailHalfLengthM*100,42.5,9));
        auto* Frame=Side==0?LeftArmCollider.Get():RightArmCollider.Get();
        Frame->SetBoxExtent(FVector(1300,55,85));
        auto* Visual=Side==0?LeftRailVisual.Get():RightRailVisual.Get();
        Visual->SetupAttachment(Rail);Visual->SetStaticMesh(Cube.Object);Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Visual->SetRelativeScale3D(FVector(26,.85,.18));
        for(auto* Joint:{Side==0?LeftHinge.Get():RightHinge.Get(),Side==0?LeftSuspension.Get():RightSuspension.Get()})
            Joint->SetupAttachment(Carriage);
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
    if(!bMechanismInitialized)PlaceMechanism(ArmClosure);
}

FVector ASuperHeavyLaunchTower::GetCaptureBaseWorld() const
{ return GetActorTransform().TransformPosition(CaptureOffsetM*100); }

void ASuperHeavyLaunchTower::SetArmClosure(double Value)
{
    const bool Changed=FMath::Abs(CommandedClosure-FMath::Clamp(Value,0.,1.))>1.e-5;
    CommandedClosure=FMath::Clamp(Value,0.,1.);
    if(!bMechanismInitialized){ArmClosure=CommandedClosure;return;}
    const double Angle=RecoveryTowerGeometry::AngleDeg(CommandedClosure,RecoveryTowerGeometry::HalfArmM-RecoveryContactGeometry::RailCentreOffsetM);
    // Commands enter the finite torque drive. They never assign a body pose.
    LeftHinge->SetAngularOrientationTarget(FRotator(0,-Angle,0));
    RightHinge->SetAngularOrientationTarget(FRotator(0,Angle,0));
    // A new hydraulic command must wake an island that slept during coast.
    // Waking retains the current pose and momentum; the motor still supplies
    // all subsequent motion through its finite torque drive.
    if(Changed)for(auto* Body:{LeftArmCollider.Get(),RightArmCollider.Get(),LeftRail.Get(),RightRail.Get()})Body->WakeAllRigidBodies();
}

void ASuperHeavyLaunchTower::PlaceMechanism(double Closure)
{
    // Migrate the former Blueprint hierarchy before attaching visuals to bodies.
    // Old instances parented these colliders to the scaled arm artwork.
    for(auto* Box:{LeftArmCollider.Get(),RightArmCollider.Get(),LeftRail.Get(),RightRail.Get()})
    {
        Box->SetSimulatePhysics(false);Box->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Box->AttachToComponent(Carriage,FAttachmentTransformRules::KeepWorldTransform);
        Box->SetWorldScale3D(FVector::OneVector);
    }
    const double Lever=RecoveryTowerGeometry::HalfArmM-RecoveryContactGeometry::RailCentreOffsetM;
    const double Angle=RecoveryTowerGeometry::AngleDeg(Closure,Lever);
    const double PivotX=CaptureOffsetM.X-Lever;
    for(int Side=0;Side<2;++Side)
    {
        const double Sign=Side==0?-1.:1.;
        const FQuat Q=GetActorQuat()*FRotator(0,Sign*Angle,0).Quaternion();
        const FVector Hinge=GetActorTransform().TransformPosition(FVector(PivotX,CaptureOffsetM.Y+Sign*RecoveryTowerGeometry::ClosedGapM,CaptureOffsetM.Z+ArmContactHeightAboveBaseM-RecoveryTowerGeometry::FrameDropM)*100);
        auto* Frame=Side==0?LeftArmCollider.Get():RightArmCollider.Get();
        auto* Rail=Side==0?LeftRail.Get():RightRail.Get();
        auto* Arm=Side==0?LeftArm.Get():RightArm.Get();
        const FVector Centre=Hinge+Q.GetForwardVector()*RecoveryTowerGeometry::HalfArmM*100;
        for(auto* Box:{Frame,Rail}){Box->SetSimulatePhysics(false);Box->SetWorldScale3D(FVector::OneVector);}
        Frame->SetWorldLocationAndRotation(Centre,Q,false,nullptr,ETeleportType::TeleportPhysics);
        Rail->SetWorldLocationAndRotation(Centre+Q.GetUpVector()*RecoveryTowerGeometry::RailCentreHeightM*100,Q,false,nullptr,ETeleportType::TeleportPhysics);
        Arm->AttachToComponent(Frame,FAttachmentTransformRules::KeepRelativeTransform);
        Arm->SetRelativeLocationAndRotation(FVector::ZeroVector,FQuat::Identity);Arm->SetRelativeScale3D(FVector(26,1.1,1.7));
        auto* Joint=Side==0?LeftHinge.Get():RightHinge.Get();
        Joint->SetWorldLocationAndRotation(Hinge,GetActorQuat());
        (Side==0?LeftSuspension.Get():RightSuspension.Get())->SetWorldLocationAndRotation(Rail->GetComponentLocation(),Q);
    }
    ArmClosure=CommandedClosure=Closure;
}

void ASuperHeavyLaunchTower::ConfigureMechanism()
{
    for(int Side=0;Side<2;++Side)
    {
        auto* Frame=Side==0?LeftArmCollider.Get():RightArmCollider.Get();
        auto* Rail=Side==0?LeftRail.Get():RightRail.Get();
        auto* Hinge=Side==0?LeftHinge.Get():RightHinge.Get();
        auto* Suspension=Side==0?LeftSuspension.Get():RightSuspension.Get();
        Frame->SetMassOverrideInKg(NAME_None,Mechanics.ArmMassKg,true);Rail->SetMassOverrideInKg(NAME_None,Mechanics.RailMassKg,true);
        for(auto* Box:{Frame,Rail})
        {
            Box->SetLinearDamping(0);Box->SetAngularDamping(0);Box->SetSimulatePhysics(true);
            Box->SetPhysicsLinearVelocity(FVector::ZeroVector);Box->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        }
        Hinge->SetDisableCollision(true);Hinge->SetProjectionEnabled(false);
        Hinge->ConstraintInstance.DisableMassConditioning();
        Hinge->ConstraintInstance.SetAngularDriveAccelerationMode(false);
        Hinge->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked,0);Hinge->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked,0);Hinge->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked,0);
        Hinge->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked,0);
        Hinge->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked,0);
        Hinge->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited,25);
        Hinge->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
        Hinge->SetOrientationDriveTwistAndSwing(false,true);Hinge->SetAngularVelocityDriveTwistAndSwing(false,true);
        Hinge->SetAngularDriveParams(Mechanics.MotorStiffnessNmRad*10000,Mechanics.MotorDampingNmsRad*10000,Mechanics.MotorTorqueNm*10000);
        Hinge->SetLinearBreakable(true,Mechanics.HingeBreakForceN*100);
        Hinge->SetAngularBreakable(true,Mechanics.HingeBreakTorqueNm*10000);
        // World is the fixed carriage bearing. The arm is a dynamic rigid body.
        Hinge->SetConstrainedComponents(Frame,NAME_None,nullptr,NAME_None);
        // Constraint frames use the closed arm orientation as their zero.
        Hinge->SetConstraintReferenceOrientation(EConstraintFrame::Frame1,FVector::ForwardVector,FVector::RightVector);
        Suspension->SetDisableCollision(true);Suspension->SetProjectionEnabled(false);
        Suspension->ConstraintInstance.DisableMassConditioning();
        Suspension->ConstraintInstance.SetLinearDriveAccelerationMode(false);
        Suspension->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked,0);Suspension->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked,0);
        Suspension->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited,Mechanics.RailTravelM*100);
        Suspension->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked,0);Suspension->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked,0);Suspension->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked,0);
        Suspension->SetLinearPositionDrive(false,false,true);Suspension->SetLinearVelocityDrive(false,false,true);
        Suspension->SetLinearPositionTarget(FVector::ZeroVector);Suspension->SetLinearVelocityTarget(FVector::ZeroVector);
        Suspension->SetLinearDriveParams(Mechanics.RailStiffnessNm,Mechanics.RailDampingNsm,Mechanics.RailForceLimitN*100);
        Suspension->SetLinearBreakable(true,Mechanics.RailBreakForceN*100);
        Suspension->SetConstrainedComponents(Frame,NAME_None,Rail,NAME_None);
        if(FParse::Param(FCommandLine::Get(),TEXT("TowerMechanismAudit")))
        {
        UE_LOG(LogTemp,Display,TEXT("TOWER_SETUP side=%d closure=%.2f arm=%s extent=%s mass=%.1f rail=%s mass=%.1f hinge1=%s hinge2=%s suspension1=%s suspension2=%s"),
            Side,CommandedClosure,*Frame->GetComponentTransform().ToString(),*Frame->GetScaledBoxExtent().ToString(),Frame->GetMass(),*Rail->GetComponentTransform().ToString(),Rail->GetMass(),
            *Hinge->ConstraintInstance.Pos1.ToString(),*Hinge->ConstraintInstance.Pos2.ToString(),*Suspension->ConstraintInstance.Pos1.ToString(),*Suspension->ConstraintInstance.Pos2.ToString());
        UE_LOG(LogTemp,Display,TEXT("TOWER_BODY side=%d arm=%s rail=%s railextent=%s"),Side,*Frame->BodyInstance.GetUnrealWorldTransform().ToString(),*Rail->BodyInstance.GetUnrealWorldTransform().ToString(),*Rail->GetScaledBoxExtent().ToString());
        }
    }
    SetArmClosure(CommandedClosure);
}

void ASuperHeavyLaunchTower::BeginPlay()
{
    Super::BeginPlay();if(!bMechanismInitialized)ResetMechanism(0);
    auto* Amber=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_SafetyAmber);
    LeftRailVisual->SetMaterial(0,Amber);RightRailVisual->SetMaterial(0,Amber);
    BuildMechanismVisuals();UpdateMechanismVisuals();
}

void ASuperHeavyLaunchTower::ResetMechanism(double InitialClosure)
{
    for(auto* Joint:{LeftHinge.Get(),RightHinge.Get(),LeftSuspension.Get(),RightSuspension.Get()})Joint->BreakConstraint();
    PlaceMechanism(FMath::Clamp(InitialClosure,0.,1.));
    bMechanismInitialized=true;BrokenRailMask=BrokenHingeMask=0;PeakRailLoadN=FVector2D::ZeroVector;
    ConfigureMechanism();
}

bool ASuperHeavyLaunchTower::IsMechanismDynamic() const
{return LeftArmCollider->IsSimulatingPhysics() && RightArmCollider->IsSimulatingPhysics() && LeftRail->IsSimulatingPhysics() && RightRail->IsSimulatingPhysics();}

void ASuperHeavyLaunchTower::Tick(float Dt)
{
    Super::Tick(Dt);if(!bMechanismInitialized)return;
    double Measured=0;
    for(int Side=0;Side<2;++Side)
    {
        const auto* Frame=Side==0?LeftArmCollider.Get():RightArmCollider.Get();
        const auto* Rail=Side==0?LeftRail.Get():RightRail.Get();
        auto* Hinge=Side==0?LeftHinge.Get():RightHinge.Get();
        auto* Suspension=Side==0?LeftSuspension.Get():RightSuspension.Get();
        const double Yaw=(GetActorQuat().Inverse()*Frame->GetComponentQuat()).Rotator().Yaw;
        Measured+=RecoveryTowerGeometry::Closure(Yaw,RecoveryTowerGeometry::HalfArmM-RecoveryContactGeometry::RailCentreOffsetM)*.5;
        RailCompressionM[Side]=(RecoveryTowerGeometry::RailCentreHeightM-FVector::DotProduct(Rail->GetComponentLocation()-Frame->GetComponentLocation(),Frame->GetUpVector())*.01);
        // Loads and persistent break flags arrive from every fixed solver step.
        // GetConstraintForce exposes only the latest impulse and can miss peaks.
        if(FParse::Param(FCommandLine::Get(),TEXT("TowerMechanismAudit")) && GFrameCounter%6==0)
        {
            FVector Force,Torque;Suspension->GetConstraintForce(Force,Torque);
            UE_LOG(LogTemp,Display,TEXT("TOWER_STEP side=%d arm=%s rail=%s force=%s torque=%s"),Side,*Frame->GetComponentTransform().ToString(),*Rail->GetComponentTransform().ToString(),*Force.ToString(),*Torque.ToString());
        }
        if(Suspension->IsBroken() && !(BrokenRailMask&(1<<Side)))
        {BrokenRailMask|=1<<Side;UE_LOG(LogTemp,Display,TEXT("TOWER_FAILURE rail=%d force_n=%.0f frame=%s railpos=%s"),Side,RailLoadN[Side],*Frame->GetComponentLocation().ToString(),*Rail->GetComponentLocation().ToString());}
        if(Hinge->IsBroken() && !(BrokenHingeMask&(1<<Side)))
        {BrokenHingeMask|=1<<Side;UE_LOG(LogTemp,Display,TEXT("TOWER_FAILURE hinge=%d frame=%s"),Side,*Frame->GetComponentLocation().ToString());}
    }
    ArmClosure=Measured;
    UpdateMechanismVisuals();
}

bool ASuperHeavyLaunchTower::IsSupport(const UPrimitiveComponent* Component,int32& Side) const
{
    if(Component==LeftRail) { Side=0;return true; }
    if(Component==RightRail) { Side=1;return true; }
    return false;
}

void ASuperHeavyLaunchTower::Release() { CaptureConstraint->BreakConstraint(); SetArmClosure(0); }

#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Flight/RecoveryMassProperties.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Components/BoxComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

namespace
{
    constexpr double StageBaseHeightM=71.02;
    constexpr double StageCentreAboveBaseM=25.;
}

UPrimitiveComponent* ASuperHeavyRecoveryDirector::GetUpperStageBody() const { return UpperStageBody; }

FTransform ASuperHeavyRecoveryDirector::GetUpperStageBaseTransform() const
{
    if(bSeparated && UpperStageBody)
        return FTransform(UpperStageBody->GetComponentQuat(),UpperStageBody->GetComponentLocation()-UpperStageBody->GetUpVector()*StageCentreAboveBaseM*100);
    return Body?FTransform(Body->GetComponentQuat(),FlightGeometry::BoosterBaseCm(*Body)+Body->GetUpVector()*StageBaseHeightM*100):FTransform::Identity;
}

void ASuperHeavyRecoveryDirector::ResetPhysicalActuators()
{
    EngineParameters.MinimumThrottle=RuntimeProfile->MinimumThrottle;
    EngineParameters.OpeningTimeConstantS=RuntimeProfile->ThrottleTimeConstant;
    EngineParameters.ShutdownTimeS=RuntimeProfile->EngineShutdownTimeS;
    EngineParameters.MaximumGimbalDeg=RuntimeProfile->MaxGimbalDeg;
    EngineParameters.GimbalRateDegS=RuntimeProfile->GimbalRateDegS;
    for(auto& Engine:Engines)
    {Engine.ThrustN=0;Engine.StepImpulseNs=0;Engine.StepForceBodyN=FVector::ZeroVector;Engine.DirectionBody=FVector::UpVector;}
    ReactionForcesBodyN.Init(FVector::ZeroVector,6);
    PeakEngineForceRatio=PeakAppliedGimbalDeg=SeparationVelocityErrorMps=0;
    SeparationMomentumRelativeError=SeparationAngularMomentumRelativeError=0;
    UpperStageThrustN=UpperStageFuelConsumedKg=0;
    ConditioningFlowKgS=FVector2D::ZeroVector;ConditioningVentedKg=GroundSupplyKg=0;
    UpperStagePropellantKg=FMath::Max(0.,RuntimeProfile->UpperStageMassKg-RuntimeProfile->UpperStageDryMassKg);
    if(!UpperStageBody)
    {
        UpperStageBody=NewObject<UBoxComponent>(this,TEXT("StarshipPhysicalBody"));
        UpperStageBody->SetBoxExtent(FVector(445,445,2400));
        UpperStageBody->SetCollisionProfileName(TEXT("PhysicsActor"));
        UpperStageBody->SetGenerateOverlapEvents(false);
        UpperStageBody->BodyInstance.bUseCCD=true;
        UpperStageBody->SetLinearDamping(0);UpperStageBody->SetAngularDamping(0);
        UpperStageBody->RegisterComponent();AddInstanceComponent(UpperStageBody);
    }
    UpperStageBody->SetSimulatePhysics(false);
    UpperStageBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(!LaunchHoldDown)
    {
        LaunchHoldDown=NewObject<UPhysicsConstraintComponent>(this,TEXT("LaunchMountHoldDown"));
        LaunchHoldDown->RegisterComponent();AddInstanceComponent(LaunchHoldDown);
        LaunchHoldDown->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked,0);
        LaunchHoldDown->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked,0);
        LaunchHoldDown->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked,0);
        LaunchHoldDown->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked,0);
        LaunchHoldDown->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked,0);
        LaunchHoldDown->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked,0);
        LaunchHoldDown->SetProjectionEnabled(false);
        LaunchHoldDown->SetDisableCollision(true);
        // Estimated mount assembly strength, in Chaos force/torque units.
        LaunchHoldDown->SetLinearBreakable(true,2.e10f);
        LaunchHoldDown->SetAngularBreakable(true,1.e13f);
    }
    LaunchHoldDown->BreakConstraint();
    LaunchHoldDown->SetWorldLocation(LaunchWorldM*100);
    Body->SetSimulatePhysics(true);Body->SetEnableGravity(true);
    Body->BodyInstance.SetGyroscopicTorqueEnabled(true);
    UpdateMass();
    LaunchHoldDown->SetConstrainedComponents(Body,NAME_None,nullptr,NAME_None);
    bLaunchHoldReleased=false;
}

void ASuperHeavyRecoveryDirector::ReleaseLaunchHoldDown()
{
    if(LaunchHoldDown)LaunchHoldDown->BreakConstraint();
    bLaunchHoldReleased=true;
}

void ASuperHeavyRecoveryDirector::SeparateUpperStage()
{
    if(bSeparated || !Body || !UpperStageBody)return;
    const FTransform StageBase=GetUpperStageBaseTransform();
    const FVector StageCentre=StageBase.GetLocation()+StageBase.GetRotation().GetUpVector()*StageCentreAboveBaseM*100;
    const FVector PointVelocity=Body->GetPhysicsLinearVelocityAtPoint(StageCentre);
    const FVector AngularVelocity=Body->GetPhysicsAngularVelocityInRadians();
    const FVector BeforeCentre=Body->GetCenterOfMass()/100.;
    const FVector BeforeMomentum=Body->GetPhysicsLinearVelocity()/100.*MassKg;
    const FQuat Q=Body->GetComponentQuat();
    const FVector OmegaBody=Q.UnrotateVector(AngularVelocity);
    const FVector BeforeAngularMomentum=Q.RotateVector(Body->GetInertiaTensor()/10000.*OmegaBody);
    const auto BoosterMass=RecoveryMass::Booster(*RuntimeProfile,PropellantKg,RcsPropellantKg,false);
    const FVector BoosterCentre=(BasePositionM+Body->GetUpVector()*BoosterMass.CentreFromBaseM)*100;
    const FVector BoosterPointVelocity=Body->GetPhysicsLinearVelocityAtPoint(BoosterCentre);
    // This is the initial state of the newly independent body. There is no kick
    // or later pose writer: upper-stage engine forces create relative motion.
    UpperStageBody->SetWorldLocationAndRotation(StageCentre,StageBase.GetRotation(),false,nullptr,ETeleportType::TeleportPhysics);
    UpperStageBody->SetMassOverrideInKg(NAME_None,RuntimeProfile->UpperStageMassKg,true);
    UpperStageBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    UpperStageBody->SetSimulatePhysics(true);UpperStageBody->SetEnableGravity(true);
    UpperStageBody->BodyInstance.SetGyroscopicTorqueEnabled(true);
    RecoveryMass::Apply(*UpperStageBody,RecoveryMass::UpperStage(RuntimeProfile->UpperStageMassKg),StageCentreAboveBaseM);
    UpperStageBody->SetPhysicsLinearVelocity(PointVelocity);
    UpperStageBody->SetPhysicsAngularVelocityInRadians(AngularVelocity);
    SeparationVelocityErrorMps=(UpperStageBody->GetPhysicsLinearVelocity()-PointVelocity).Size()/100.;
    bSeparated=true;UpdateMass();SeparationMassKg=MassKg;
    // Transport the same rigid-stack velocity field to the two new centres of
    // mass. This is a separation initial condition, never a guidance correction.
    Body->SetPhysicsLinearVelocity(BoosterPointVelocity);
    const FVector BoosterMomentum=Body->GetPhysicsLinearVelocity()/100.*MassKg;
    const FVector ShipMomentum=UpperStageBody->GetPhysicsLinearVelocity()/100.*RuntimeProfile->UpperStageMassKg;
    const FVector AfterAngularMomentum=Q.RotateVector((Body->GetInertiaTensor()+UpperStageBody->GetInertiaTensor())/10000.*OmegaBody)+
        FVector::CrossProduct(Body->GetCenterOfMass()/100.-BeforeCentre,BoosterMomentum)+
        FVector::CrossProduct(UpperStageBody->GetCenterOfMass()/100.-BeforeCentre,ShipMomentum);
    SeparationMomentumRelativeError=(BoosterMomentum+ShipMomentum-BeforeMomentum).Size()/FMath::Max(1.,BeforeMomentum.Size());
    SeparationAngularMomentumRelativeError=(AfterAngularMomentum-BeforeAngularMomentum).Size()/FMath::Max(1.,BeforeAngularMomentum.Size());
}

void ASuperHeavyRecoveryDirector::TickUpperStage(double Dt)
{
    if(!bSeparated || !UpperStageBody || !UpperStageBody->IsSimulatingPhysics())return;
    const FVector Position=UpperStageBody->GetComponentLocation();
    const double Height=FlightGeometry::AltitudeM(Position);
    const auto Air=RecoveryAtmosphere::Sample(Height,RuntimeProfile->SeaLevelTemperatureOffsetK);
    const FVector Velocity=UpperStageBody->GetPhysicsLinearVelocity()/100.;
    const FVector Relative=Velocity-WindAt(Height);
    const FVector Local=UpperStageBody->GetComponentQuat().UnrotateVector(Relative);
    const double Speed=FMath::Max(1.,Relative.Size());
    const double Q=.5*Air.Density*Relative.SizeSquared();
    const FVector Drag=-Q*64*.6*Relative/Speed;
    const FVector Side=UpperStageBody->GetComponentQuat().RotateVector(-Q*450*FVector(Local.X,Local.Y,0)/Speed);
    UpperStageBody->AddForce((Drag+Side)*100);
    const double Target=UpperStagePropellantKg>0 ? 6*RuntimeProfile->UpperStageEngineThrustN : 0.;
    UpperStageThrustN=FMath::Lerp(UpperStageThrustN,Target,1-FMath::Exp(-Dt/.4));
    UpperStageThrustN=RecoveryActuators::FuelLimitedThrust(UpperStageThrustN,UpperStagePropellantKg,RuntimeProfile->UpperStageIspS,Dt);
    const double Used=UpperStageThrustN/(RuntimeProfile->UpperStageIspS*RecoveryAtmosphere::G0)*Dt;
    UpperStagePropellantKg=FMath::Max(0.,UpperStagePropellantKg-Used);UpperStageFuelConsumedKg+=Used;
    const double Mass=RuntimeProfile->UpperStageDryMassKg+UpperStagePropellantKg;
    RecoveryMass::Apply(*UpperStageBody,RecoveryMass::UpperStage(Mass),StageCentreAboveBaseM);
    const FVector RadialDown=(FlightGeometry::EarthCenterCm()-Position).GetSafeNormal();
    UpperStageBody->AddForce((RadialDown*Air.Gravity-FVector(0,0,GetWorld()->GetGravityZ()/100.))*Mass*100);
    const FTransform StageBase=GetUpperStageBaseTransform();
    // Symmetric sea-level and vacuum groups; fixed directions preserve angular
    // momentum in vacuum. Orbital GNC and tank inertias are separate work items.
    for(const FVector& PositionM:FlightGeometry::UpperStageNozzlePositionsM())
    {
        const FVector Nozzle=StageBase.TransformPosition(PositionM*100);
        UpperStageBody->AddForceAtLocation(UpperStageBody->GetUpVector()*(UpperStageThrustN/6.)*100,Nozzle);
    }
}

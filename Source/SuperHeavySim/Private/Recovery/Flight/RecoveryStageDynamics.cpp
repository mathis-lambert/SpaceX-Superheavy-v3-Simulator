#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Flight/RecoveryMassProperties.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Components/BoxComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

UPrimitiveComponent* ASuperHeavyRecoveryDirector::GetUpperStageBody() const { return UpperStageBody; }

FTransform ASuperHeavyRecoveryDirector::GetUpperStageBaseTransform() const
{
    if(bSeparated && UpperStageBody)
        return FTransform(UpperStageBody->GetComponentQuat(),UpperStageBody->GetComponentLocation()-UpperStageBody->GetUpVector()*FlightGeometry::UpperStageCentreFromBaseM*100);
    return Body?FTransform(Body->GetComponentQuat(),FlightGeometry::BoosterBaseCm(*Body)+Body->GetUpVector()*FlightGeometry::UpperStageBaseHeightM*100):FTransform::Identity;
}

void ASuperHeavyRecoveryDirector::ResetPhysicalActuators()
{
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
    // Allocate/activate the proxy on the game thread. The solver initializes
    // both separated bodies atomically before their first independent step.
    const FTransform StageBase=GetUpperStageBaseTransform();
    const FVector StageCentre=StageBase.GetLocation()+StageBase.GetRotation().GetUpVector()*FlightGeometry::UpperStageCentreFromBaseM*100.;
    UpperStageBody->SetWorldLocationAndRotation(StageCentre,StageBase.GetRotation(),false,nullptr,ETeleportType::TeleportPhysics);
    UpperStageBody->SetMassOverrideInKg(NAME_None,RuntimeProfile->UpperStageMassKg,true);
    UpperStageBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    UpperStageBody->SetSimulatePhysics(true);UpperStageBody->SetEnableGravity(true);
    UpperStageBody->BodyInstance.SetGyroscopicTorqueEnabled(true);
    RecoveryMass::Apply(*UpperStageBody,RecoveryMass::UpperStage(RuntimeProfile->UpperStageMassKg),FlightGeometry::UpperStageCentreFromBaseM);
    bSeparated=true;
    // Publish in this same external frame so proxy creation and its initial
    // conditions reach the solver together, including TG_PostPhysics delivery.
    SubmitDynamicsCommand();
}

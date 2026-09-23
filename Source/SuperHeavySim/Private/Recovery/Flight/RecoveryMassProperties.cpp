#include "Recovery/Flight/RecoveryMassProperties.h"
#include "Components/PrimitiveComponent.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "PhysicsEngine/BodyInstance.h"

void RecoveryMass::Apply(UPrimitiveComponent& Body,const FProperties& Properties,double BodyOriginFromBaseM)
{
    auto* Instance=Body.GetBodyInstance();
    if(!Instance)return;
    // Component scaling is already baked into Chaos geometry. These values are
    // physical centimetres and kg cm², not scaled Blueprint-local dimensions.
    FPhysicsCommand::ExecuteWrite(Instance->ActorHandle,[&](const FPhysicsActorHandle& Actor)
    {
        FPhysicsInterface::SetMass_AssumesLocked(Actor,float(Properties.MassKg));
        FPhysicsInterface::SetMassSpaceInertiaTensor_AssumesLocked(Actor,Properties.InertiaKgM2*10000.);
        FPhysicsInterface::SetComLocalPose_AssumesLocked(Actor,FTransform(FQuat::Identity,FVector(0,0,(Properties.CentreFromBaseM-BodyOriginFromBaseM)*100.)));
    });
}

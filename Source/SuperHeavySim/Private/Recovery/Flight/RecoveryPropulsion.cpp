#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Vehicle/SuperHeavyVehicleActor.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::InitializePhysicalActuators()
{
    Engines.Reset();
    const FQuat Q=Body->GetComponentQuat();
    const FVector Base=FlightGeometry::BoosterBaseCm(*Body);
    TInlineComponentArray<UChildActorComponent*> EngineChildren(Vehicle);
    for(auto* Child:EngineChildren)
    {
        const FString Id=Child->GetName();
        if(!Id.StartsWith(TEXT("R")) || !Child->GetChildActor()) continue;
        TInlineComponentArray<USceneComponent*> Parts(Child->GetChildActor());
        USceneComponent* Pivot=nullptr;
        for(auto* Part:Parts)if(Part->GetName()==TEXT("GimbalPivot"))Pivot=Part;
        for(auto* Part:Parts) if(Part->GetName()==TEXT("ThrustSocket"))
        {
            FRecoveryEngineState Engine;
            Engine.Id=Child->GetFName();
            const FVector Mount=Pivot?Pivot->GetComponentLocation():Part->GetComponentLocation();
            Engine.PositionFromBaseM=Q.UnrotateVector(Mount-Base)/100.;
            Engine.NozzleOffsetBodyM=Q.UnrotateVector(Part->GetComponentLocation()-Mount)/100.;
            Engine.bGimballed=Id.StartsWith(TEXT("RG"));
            Engine.bCentral=Id.StartsWith(TEXT("RGC"));
            Engines.Add(Engine);
            break;
        }
    }
    ReactionForcesBodyN.Init(FVector::ZeroVector,6);
    UE_LOG(LogTemp,Display,TEXT("PHYSICAL_ENGINE_REGISTRY count=%d"),Engines.Num());
}

void ASuperHeavyRecoveryDirector::SetFlightCommand(const FVector& ThrustAcceleration,const FVector& TargetUp)
{
    DynamicsCommand.ThrustAccelerationMps2=ThrustAcceleration;
    DynamicsCommand.TargetUpWorld=TargetUp;
}

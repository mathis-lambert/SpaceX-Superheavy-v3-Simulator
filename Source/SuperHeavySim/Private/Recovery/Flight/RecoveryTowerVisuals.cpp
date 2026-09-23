#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

void ASuperHeavyLaunchTower::BuildMechanismVisuals()
{
    if(!RailActuatorVisuals.IsEmpty())return;
    auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Graphite=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_Graphite);
    auto* Metal=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_Cladding);
    for(int32 I=0;I<12;++I)
    {
        auto* Part=NewObject<UStaticMeshComponent>(this,FName(*FString::Printf(TEXT("RailHydraulic_%d"),I)));
        Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Cylinder);
        Part->SetMaterial(0,I%2==0?Graphite:Metal);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);Part->RegisterComponent();AddInstanceComponent(Part);RailActuatorVisuals.Add(Part);
    }
}

void ASuperHeavyLaunchTower::UpdateMechanismVisuals()
{
    if(RailActuatorVisuals.Num()!=12)return;
    for(int32 Side=0;Side<2;++Side)
    {
        const auto* Frame=Side==0?LeftArmCollider.Get():RightArmCollider.Get();
        const auto* Rail=Side==0?LeftRail.Get():RightRail.Get();
        for(int32 Station=0;Station<3;++Station)
        {
            const double Along=(Station-1)*800.;
            const FVector A=Frame->GetComponentTransform().TransformPosition(FVector(Along,0,65));
            const FVector B=Rail->GetComponentTransform().TransformPosition(FVector(Along,0,-9));
            const FVector Delta=B-A,Axis=Delta.GetSafeNormal();const FQuat Q=FRotationMatrix::MakeFromZ(Axis).ToQuat();
            const int32 I=Side*6+Station*2;
            RailActuatorVisuals[I]->SetWorldLocationAndRotation(A+Axis*19,Q);RailActuatorVisuals[I]->SetWorldScale3D(FVector(.3,.3,.38));
            const double RodLength=FMath::Max(0.,Delta.Size()-25.);
            RailActuatorVisuals[I+1]->SetVisibility(RodLength>0 && !(BrokenRailMask&(1<<Side)));
            RailActuatorVisuals[I+1]->SetWorldLocationAndRotation(A+Axis*(25+RodLength*.5),Q);
            RailActuatorVisuals[I+1]->SetWorldScale3D(FVector(.12,.12,RodLength*.01));
        }
    }
}

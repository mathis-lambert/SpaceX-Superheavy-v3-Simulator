#include "Recovery/Presentation/RecoverySiteActivityComponent.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"

void URecoverySiteActivityComponent::UpdateFacilityVents(const FTransform& Site,const FVector& Wind)
{
    // Reuse the non-emissive condensation material and its isotropic voxel
    // contract. Two small streams, culled at 800 m, bound volumetric cost.
    if(Vents.IsEmpty())
    {
        auto* Source=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_CryogenicVapor);
        if(!Source)return;
        for(int I=0;I<2;++I)
        {
            auto* V=NewObject<UHeterogeneousVolumeComponent>(GetOwner(),FName(*FString::Printf(TEXT("TankConditioning_%d"),I)));
            auto* M=UMaterialInstanceDynamic::Create(Source,this);
            V->SetMaterial(0,M);V->SetMobility(EComponentMobility::Movable);
            V->SetVolumeResolution(FIntVector(48,48,68));V->bPivotAtCentroid=true;
            V->StepFactor=1;V->ShadowStepFactor=2;V->LightingDownsampleFactor=2;
            V->SetWorldScale3D(FVector(50));V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->SetCastShadow(true);
            V->RegisterComponent();GetOwner()->AddInstanceComponent(V);Vents.Add(V);VentMaterials.Add(M);
        }
    }
    const auto Colour=[](FVector V){return FLinearColor(V.X,V.Y,V.Z);};
    const auto* PC=GetWorld()->GetFirstPlayerController();const auto* View=PC?PC->GetViewTarget():nullptr;
    for(int I=0;I<Vents.Num();++I)
    {
        const FVector Vent=Site.TransformPosition(FVector(-9700+I*6000,7000,1520));
        const double Distance=View?FVector::Dist(View->GetActorLocation(),Vent)/100:1000;
        const float Fade=1-FMath::SmoothStep(550.,800.,Distance);
        Vents[I]->SetVisibility(Fade>.001f);if(Fade<=.001f)continue;
        Vents[I]->SetWorldLocation(Vent+FVector(150,0,-1200));
        auto* M=VentMaterials[I].Get();M->SetVectorParameterValue(TEXT("VentPosition"),Colour(Vent));
        M->SetVectorParameterValue(TEXT("Up"),FLinearColor(0,0,1));M->SetVectorParameterValue(TEXT("Outward"),FLinearColor(0,-1,0));
        // Contained boil-off moves gently with the local wind at the tank farm.
        M->SetVectorParameterValue(TEXT("Wind"),Colour(Wind.GetClampedToMaxSize(3)*100));
        M->SetScalarParameterValue(TEXT("FlowTime"),ActivityTime+I*9.2);
        M->SetScalarParameterValue(TEXT("FlowStrength"),Fade*(.12+.06*FMath::Sin(ActivityTime*.12+I)));
        M->SetScalarParameterValue(TEXT("MetersPerVoxel"),.5f);
    }
}

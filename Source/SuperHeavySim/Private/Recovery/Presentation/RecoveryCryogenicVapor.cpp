#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

int32 URecoveryVaporComponent::GetCryogenicVolumeCount() const
{int32 Count=0;for(const auto& V:CryogenicVolumes)if(V && V->IsVisible())++Count;return Count;}

void URecoveryVaporComponent::UpdateCryogenic(float Dt,const ASuperHeavyRecoveryDirector& D)
{
    if(CryogenicVolumes.IsEmpty())
    {
        auto* Source=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_CryogenicVapor);
        if(!Source)return;
        for(int I=0;I<2;++I)
        {
            auto* V=NewObject<UHeterogeneousVolumeComponent>(GetOwner(),FName(*FString::Printf(TEXT("CryogenicFlow_%d"),I)));
            auto* M=UMaterialInstanceDynamic::Create(Source,this);
            V->SetMobility(EComponentMobility::Movable);V->SetMaterial(0,M);
            V->SetVolumeResolution(FIntVector(48,48,128));V->bPivotAtCentroid=true;
            V->StepFactor=1;V->ShadowStepFactor=2;V->LightingDownsampleFactor=2;
            V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->SetCastShadow(true);
            V->SetVisibility(false);V->RegisterComponent();GetOwner()->AddInstanceComponent(V);
            CryogenicVolumes.Add(V);CryogenicMaterials.Add(M);
        }
    }
    FlowTime+=Dt;
    const FVector2D Flow=D.GetConditioningFlowKgS();
    const FQuat Q=D.GetBody()->GetComponentQuat();
    const FVector Up=Q.GetUpVector(),Base=FlightGeometry::BoosterBaseCm(*D.GetBody());
    const FVector Wind=D.GetWindVelocityMps(30)*100;
    const auto& Points=FlightGeometry::ConditioningVentPositionsM();
    const auto& Directions=FlightGeometry::ConditioningVentDirections();
    const auto Colour=[](FVector V){return FLinearColor(V.X,V.Y,V.Z);};
    for(int I=0;I<2;++I)
    {
        CryogenicStrength[I]=FMath::FInterpTo(CryogenicStrength[I],FMath::Sqrt(Flow[I]),Dt,Flow[I]>.01?1.:.7);
        auto* V=CryogenicVolumes[I].Get();auto* M=CryogenicMaterials[I].Get();
        V->SetVisibility(CryogenicStrength[I]>.005 && D.AltitudeM<200);
        if(!V->IsVisible())continue;
        const FVector Vent=Base+Q.RotateVector(Points[I])*100;
        const FVector Out=Q.RotateVector(Directions[I]).GetSafeNormal();
        // Cover the complete downstream path, including crosswind. A fixed box
        // clips the long tail when a live wind experiment increases advection.
        constexpr double TailTravel=28./2.5;
        const FVector Drift=Wind*(TailTravel-.9*(1-FMath::Exp(-TailTravel/.9)))*.2;
        const FVector LocalDrift=Q.UnrotateVector(Drift).GetAbs();
        const FVector Centre=Vent-Up*1350+Out*250+Drift*.5;
        V->SetWorldLocationAndRotation(Centre,Q);
        // Heterogeneous component bounds are expressed in voxel dimensions.
        V->SetWorldScale3D(FVector((2400+LocalDrift.X)/48,(2400+LocalDrift.Y)/48,(3200+LocalDrift.Z)/128));
        M->SetVectorParameterValue(TEXT("VentPosition"),Colour(Vent));
        M->SetVectorParameterValue(TEXT("Up"),Colour(Up));M->SetVectorParameterValue(TEXT("Outward"),Colour(Out));
        M->SetVectorParameterValue(TEXT("Wind"),Colour(Wind));
        M->SetScalarParameterValue(TEXT("FlowTime"),FlowTime);
        M->SetScalarParameterValue(TEXT("FlowStrength"),CryogenicStrength[I]);
    }
}

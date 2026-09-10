#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Shared/RecoveryLog.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

int32 URecoveryVaporComponent::GetCryogenicVolumeCount() const
{int32 Count=0;for(const auto& V:CryogenicVolumes)if(V && V->IsVisible())++Count;return Count;}

void URecoveryVaporComponent::UpdateCryogenic(float Dt,const ASuperHeavyRecoveryDirector& D)
{
    if(!bCryogenicInitialized)
    {
        bCryogenicInitialized=true;
        auto* Source=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_CryogenicVapor);
        if(!Source){UE_LOG(LogRecovery,Error,TEXT("Required condensation material is missing: %s"),RecoveryAssets::M_CryogenicVapor);return;}
        for(int I=0;I<2;++I)
        {
            auto* V=NewObject<UHeterogeneousVolumeComponent>(GetOwner(),FName(*FString::Printf(TEXT("CryogenicFlow_%d"),I)));
            auto* M=UMaterialInstanceDynamic::Create(Source,this);
            V->SetMobility(EComponentMobility::Movable);V->SetMaterial(0,M);
            V->SetVolumeResolution(FIntVector(48,48,128));V->bPivotAtCentroid=true;
            V->StepFactor=.5;V->ShadowStepFactor=1;V->LightingDownsampleFactor=2;
            V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->SetCastShadow(true);
            V->SetVisibility(false);V->RegisterComponent();GetOwner()->AddInstanceComponent(V);
            CryogenicVolumes.Add(V);CryogenicMaterials.Add(M);
        }
    }
    if(CryogenicVolumes.IsEmpty())return;
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
        constexpr double TailTravel=44./2.5;
        const FVector Drift=Wind*(TailTravel-.9*(1-FMath::Exp(-TailTravel/.9)))*.2;
        const FVector LocalDrift=Q.UnrotateVector(Drift).GetAbs();
        const FVector Centre=Vent-Up*2150+Out*250+Drift*.5;
        V->SetWorldLocationAndRotation(Centre,Q);
        // UE integrates extinction over the local voxel ray. Keep isotropic
        // voxels and explicitly convert inverse metres to inverse local units;
        // stretching each axis made opacity depend on view and wind direction.
        constexpr double VoxelCm=50.;
        const FVector Size=FVector(2400,2400,4800)+LocalDrift;
        V->SetVolumeResolution(FIntVector(FMath::CeilToInt(Size.X/VoxelCm),FMath::CeilToInt(Size.Y/VoxelCm),FMath::CeilToInt(Size.Z/VoxelCm)));
        V->SetWorldScale3D(FVector(VoxelCm));
        M->SetVectorParameterValue(TEXT("VentPosition"),Colour(Vent));
        M->SetVectorParameterValue(TEXT("Up"),Colour(Up));M->SetVectorParameterValue(TEXT("Outward"),Colour(Out));
        M->SetVectorParameterValue(TEXT("Wind"),Colour(Wind));
        M->SetScalarParameterValue(TEXT("FlowTime"),FlowTime);
        M->SetScalarParameterValue(TEXT("FlowStrength"),CryogenicStrength[I]);
        M->SetScalarParameterValue(TEXT("MetersPerVoxel"),VoxelCm*.01);
    }
}

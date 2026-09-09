#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Components/PrimitiveComponent.h"
#include "SparseVolumeTexture/SparseVolumeTexture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

int32 URecoveryVaporComponent::GetTurbulentVolumeCount() const
{int32 Count=0;for(const auto& V:TurbulentVolumes)if(V && V->IsVisible())++Count;return Count;}

void URecoveryVaporComponent::UpdateTurbulent(float Dt,const ASuperHeavyRecoveryDirector& D,double Delivered)
{
    if(TurbulentVolumes.IsEmpty())
    {
        auto* Source=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_TurbulentDeluge);
        auto* Field=LoadObject<USparseVolumeTexture>(nullptr,RecoveryAssets::SVT_TurbulentDeluge);
        if(!Source || !Field)return;
        TurbulentFrameOffset=Field->GetFrameTransform().GetTranslation();
        TurbulentVoxelM=Field->GetFrameTransform().GetScale3D().GetMin();
        for(int I=0;I<8;++I)
        {
            auto* V=NewObject<UHeterogeneousVolumeComponent>(GetOwner(),FName(*FString::Printf(TEXT("TurbulentDeluge_%d"),I)));
            auto* M=UMaterialInstanceDynamic::Create(Source,this);
            V->SetMobility(EComponentMobility::Movable);V->SetMaterial(0,M);
            V->bPivotAtCentroid=true;V->SetPlaying(false);V->SetLooping(false);
            V->StepFactor=1;V->ShadowStepFactor=2;V->LightingDownsampleFactor=2;
            V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->SetCastShadow(true);V->SetVisibility(false);
            V->RegisterComponent();GetOwner()->AddInstanceComponent(V);
            TurbulentVolumes.Add(V);TurbulentMaterials.Add(M);TurbulentBillows.AddDefaulted();
        }
    }
    if(TurbulentGeneration!=D.GetMissionGeneration())
    {
        for(int I=0;I<TurbulentBillows.Num();++I){TurbulentBillows[I].Age=100;TurbulentVolumes[I]->SetVisibility(false);}
        TurbulentSpawnClock=0;NextTurbulent=0;TurbulentGeneration=D.GetMissionGeneration();
    }
    const FVector Base=FlightGeometry::BoosterBaseCm(*D.GetBody()),Wind=D.GetWindVelocityMps(30)*100;
    TurbulentSpawnClock+=Dt;
    if(Delivered>.003 && D.GetDelugeFlow()>.05 && D.AltitudeM<180 && TurbulentSpawnClock>.8)
    {
        const int I=NextTurbulent%TurbulentBillows.Num();auto& B=TurbulentBillows[I];
        if(B.Age>=B.Life)
        {
            const double Angle=NextTurbulent*2.399963;const FVector Radial(FMath::Cos(Angle),FMath::Sin(Angle),0);
            B=FBillow();B.Age=0;B.Life=6.;B.Seed=NextTurbulent*.618034f;
            B.Position=FVector(Base.X,Base.Y,1650)+Radial*1700;
            B.Velocity=Radial*800+Wind;B.FlowAxis=Radial;B.Density=FMath::Clamp(D.GetDelugeFlow()*1.3,.1,1.3);
            ++NextTurbulent;TurbulentSpawnClock=0;
        }
    }
    FVector Camera=Base;
    if(const auto* PC=GetWorld()->GetFirstPlayerController())if(PC->PlayerCameraManager)Camera=PC->PlayerCameraManager->GetCameraLocation();
    for(int I=0;I<TurbulentBillows.Num();++I)
    {
        auto& B=TurbulentBillows[I];auto* V=TurbulentVolumes[I].Get();auto* M=TurbulentMaterials[I].Get();
        if(B.Age>=B.Life){V->SetVisibility(false);continue;}
        B.Age+=Dt;
        B.Velocity=FMath::Lerp(B.Velocity,Wind+FVector(0,0,80),1-FMath::Exp(-Dt/3));B.Position+=B.Velocity*Dt;
        const float Fade=FMath::SmoothStep(0.f,.7f,B.Age)*(1-FMath::SmoothStep(3.8f,B.Life,B.Age));
        const float DistanceFade=1-FMath::SmoothStep(220000.f,420000.f,float(FVector::Distance(Camera,B.Position)));
        V->SetVisibility(Fade*DistanceFade>.002f);
        if(!V->IsVisible())continue;
        // Original 20 x 16 x 12 m baked domain. Uniform scaling preserves
        // isotropic extinction; instances share one streamed 64-frame cache.
        const double Expansion=2.7+B.Age*.16,ScaleCm=100*Expansion;
        const FQuat Q=FRotationMatrix::MakeFromX(B.FlowAxis).ToQuat();
        V->SetWorldLocationAndRotation(B.Position-Q.RotateVector(TurbulentFrameOffset*ScaleCm),Q);
        V->SetWorldScale3D(FVector(ScaleCm));
        V->SetFrame(FMath::Clamp(B.Age*18.f,0.f,63.f));
        V->SetStreamingMipBias(FVector::DistSquared(Camera,B.Position)>100000.*100000.?1:0);
        M->SetScalarParameterValue(TEXT("DensityScale"),B.Density*Fade*DistanceFade/Expansion);
        M->SetScalarParameterValue(TEXT("MetersPerVoxel"),TurbulentVoxelM*Expansion);
        if(FParse::Param(FCommandLine::Get(),TEXT("RecoveryDetailReview")) && B.Age>=2 && B.Age<2+Dt)
            UE_LOG(LogTemp,Display,TEXT("FLOW_FIELD index=%d frame=%.2f range=%.0f-%.0f size=%s bounds=%s centre=%s tick=%d density=%.3f"),
                I,V->Frame,V->StartFrame,V->EndFrame,*V->VolumeResolution.ToString(),*V->Bounds.BoxExtent.ToString(),*V->Bounds.Origin.ToString(),V->IsComponentTickEnabled(),M->K2_GetScalarParameterValue(TEXT("DensityScale")));
    }
}

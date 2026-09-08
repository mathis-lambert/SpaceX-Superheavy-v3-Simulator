#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "RHI.h"

URecoveryVaporComponent::URecoveryVaporComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}

void URecoveryVaporComponent::Build()
{
    // Volume-domain meshes are voxelized into the fog grid, not drawn as shells.
    // Two 3D texture samples erode their density; local lights illuminate the medium.
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Material=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_VolumetricVapor);
    if(!Mesh || !Material || Material->GetMaterial()->MaterialDomain!=MD_Volume)return;
    for(int32 I=0;I<FMath::Clamp(VolumeBudget,64,256);++I)
    {
        auto* Volume=NewObject<UStaticMeshComponent>(GetOwner());
        Volume->SetMobility(EComponentMobility::Movable);
        auto* Dynamic=UMaterialInstanceDynamic::Create(Material,this);
        Volume->SetStaticMesh(Mesh);Volume->SetMaterial(0,Dynamic);
        Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Volume->SetCastShadow(false);Volume->SetVisibleInRayTracing(false);
        Volume->SetVisibility(false);
        Volume->RegisterComponent();GetOwner()->AddInstanceComponent(Volume);
        Volumes.Add(Volume);Materials.Add(Dynamic);Billows.AddDefaulted();
    }
}

void URecoveryVaporComponent::Spawn(const FVector& Position,const FVector& Velocity,float Life,float Radius,float Growth,float Density,EVaporKind Kind)
{
    const int32 I=Next++%Billows.Num();
    Billows[I]={Position,Velocity,0,Life,Radius,Growth,Density};
    Billows[I].Kind=Kind;Billows[I].Seed=Next*.6180339f;
    Billows[I].FlowAxis=Velocity.GetSafeNormal(UE_SMALL_NUMBER,FVector::ForwardVector);
    const float Variation=.85f+.3f*FMath::Frac(Next*.381966f);
    Billows[I].ShapeScale=Kind==EVaporKind::Deluge?FVector(1.65*Variation,1.05,.72/Variation):FVector(1.7,.85,1.);
    Volumes[I]->SetVisibility(true);
}

int32 URecoveryVaporComponent::GetActiveVolumeCount() const
{
    int32 Count=0;for(const auto& B:Billows)if(B.Age<B.Life)++Count;
    return Count;
}

bool URecoveryVaporComponent::HasRenderableDensity() const
{
    for(int32 I=0;I<Volumes.Num();++I)
        if(Volumes[I]->IsVisible() && Materials[I]->GetMaterial()->MaterialDomain==MD_Volume &&
            !Materials[I]->GetMaterial()->IsCompilingOrHadCompileError(GMaxRHIShaderPlatform) &&
            Materials[I]->K2_GetScalarParameterValue(TEXT("Density"))>.01f)return true;
    return false;
}

void URecoveryVaporComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!FApp::CanEverRender()){SetComponentTickEnabled(false);return;}
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D || !D->GetBody())return;
    if(Volumes.IsEmpty())Build();
    if(Volumes.IsEmpty())return;
    if(D->GetMissionGeneration()!=LastMissionGeneration)
        for(int I=0;I<Billows.Num();++I){Billows[I].Age=100;Volumes[I]->SetVisibility(false);}
    LastMissionGeneration=D->GetMissionGeneration();
    const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody());
    const FVector Up=D->GetBody()->GetUpVector();
    const FVector Wind=D->GetWindVelocityMps(30)*100;
    UpdateCryogenic(Dt,*D);
    if(D->AltitudeM<=170)LastTrailPosition=Base;
    // Ground water flow and engine exhaust coexist with the cryogenic vents.
    // Separate clocks prevent active conditioning from suppressing the deluge.
    DelugeSpawnClock+=Dt;
    if(D->GetDelugeFlow()>.05 && D->AltitudeM<220 && DelugeSpawnClock>.25)
    {
        DelugeSpawnClock=FMath::Fmod(DelugeSpawnClock,.25);
        const bool Hot=D->ActiveEngines>0 && D->Throttle>.05;
        const int32 Count=Hot && D->ActiveEngines>=13?5:3;
        for(int32 J=0;J<Count;++J)
        {
            const double Angle=Next*2.399963;
            const FVector Radial(FMath::Cos(Angle),FMath::Sin(Angle),0);
            Spawn(FVector(Base.X,Base.Y,250)+Radial*(650+J*130),Radial*((Hot?2100:600)+J*140)+Wind+FVector(0,0,Hot?110+J*55:30),Hot?22:9,Hot?3+J*.6f:1.1,Hot?2.2f:.8f,(Hot?3.f:.6f)*D->GetDelugeFlow(),EVaporKind::Deluge);
        }
    }
    else if(D->Phase==ERecoveryPhase::Ascent && D->AltitudeM>170 && D->AltitudeM<12000 && (Base-LastTrailPosition).Size()>6500)
    {
        LastTrailPosition=Base;
        Spawn(Base-Up*5500,Wind-Up*600,22,11,2.f,1.2f*FMath::Sqrt(D->PressurePa/101325.),EVaporKind::Trail);
    }
    for(int32 I=0;I<Billows.Num();++I)
    {
        auto& B=Billows[I];if(B.Age>=B.Life)continue;
        B.Age+=Dt;
        if(B.Age>=B.Life){Volumes[I]->SetVisibility(false);continue;}
        const FVector Eddy(FMath::Sin(B.Age*.71+B.Seed),FMath::Cos(B.Age*.53+B.Seed*2),FMath::Sin(B.Age*.43+B.Seed*3)*.45);
        const double Buoyancy=180*(1-FMath::Exp(-B.Age/4));
        const FVector Ambient=Wind+Eddy*150+FVector(0,0,Buoyancy);
        B.Velocity=FMath::Lerp(B.Velocity,Ambient,1-FMath::Exp(-Dt/(B.Kind==EVaporKind::Deluge?4.:1.7)));
        B.Position+=B.Velocity*Dt;
        if(B.Kind==EVaporKind::Deluge)B.Position.Z=FMath::Max(200.,B.Position.Z);
        const float Radius=B.RadiusM+FMath::Sqrt(B.Age*2)*B.GrowthMps;
        const float Fade=FMath::SmoothStep(0.f,.6f,B.Age)*(1-FMath::SmoothStep(B.Life*.65f,B.Life,B.Age));
        Volumes[I]->SetWorldLocationAndRotation(B.Position,FRotationMatrix::MakeFromX(B.FlowAxis).ToQuat());
        Volumes[I]->SetWorldScale3D(B.ShapeScale*(Radius*2));
        Materials[I]->SetScalarParameterValue(TEXT("RadiusCm"),Radius*100);
        Materials[I]->SetVectorParameterValue(TEXT("ShapeScale"),FLinearColor(B.ShapeScale.X,B.ShapeScale.Y,B.ShapeScale.Z));
        Materials[I]->SetVectorParameterValue(TEXT("FlowAxis"),FLinearColor(B.FlowAxis.X,B.FlowAxis.Y,B.FlowAxis.Z));
        Materials[I]->SetScalarParameterValue(TEXT("Age"),B.Age);
        Materials[I]->SetScalarParameterValue(TEXT("Seed"),B.Seed);
        // A fitted condensation/dilution surrogate. This is not CFD or a claim
        // of conserved aerosol mass; vehicle propellant flow is tracked separately.
        const float Dilution=FMath::Pow(B.RadiusM/Radius,.3f);
        Materials[I]->SetScalarParameterValue(TEXT("Density"),B.Density*Fade*Dilution);
    }
}

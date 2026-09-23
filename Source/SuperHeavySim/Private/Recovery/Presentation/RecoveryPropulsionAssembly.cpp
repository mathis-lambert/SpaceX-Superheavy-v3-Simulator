#include "Recovery/Presentation/RecoveryPresentationComponent.h"
#include "Recovery/Presentation/RecoveryPropulsionVisuals.h"
#include "Recovery/Presentation/RecoveryStartupSubsystem.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Presentation/RecoveryEnvironmentProfile.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Vehicle/SuperHeavyVehicleActor.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

void URecoveryPresentationComponent::Build()
{
    auto* D = Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if (!D || !D->Vehicle || !D->GetBody())
        return;
    auto* Mesh = LoadObject<UStaticMesh>(nullptr, RecoveryAssets::SM_ExhaustEnvelope);
    auto* Material = LoadObject<UMaterialInterface>(nullptr, RecoveryAssets::M_RaptorPlume);
    if (!Mesh || !Material)
        return;
    MixingPlume = NewObject<UStaticMeshComponent>(GetOwner());
    MixingMaterial = UMaterialInstanceDynamic::Create(Material, this);
    MixingMaterial->SetScalarParameterValue(TEXT("MixingLayer"), 1);
    MixingPlume->SetStaticMesh(Mesh);
    MixingPlume->SetMaterial(0, MixingMaterial);
    MixingPlume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MixingPlume->SetCastShadow(false);
    MixingPlume->RegisterComponent();
    GetOwner()->AddInstanceComponent(MixingPlume);
    auto* FinMaterial = LoadObject<UMaterialInterface>(nullptr, RecoveryAssets::M_GridFinAlloy);
    if (auto* Steel = LoadObject<UMaterialInterface>(nullptr, RecoveryAssets::M_BoosterFlight))
    {
        TInlineComponentArray<UStaticMeshComponent*> Hull(D->Vehicle);
        for (auto* Part : Hull)
            if (Part->GetName() == TEXT("SH_Body_Mesh"))
                Part->SetMaterial(0, Steel);
    }
    NozzleCores = NewObject<UInstancedStaticMeshComponent>(GetOwner());
    NozzleCores->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    NozzleCores->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, RecoveryAssets::M_NozzleCore));
    NozzleCores->NumCustomDataFloats = 1;
    NozzleCores->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NozzleCores->SetCastShadow(false);
    NozzleCores->RegisterComponent();
    GetOwner()->AddInstanceComponent(NozzleCores);
    TInlineComponentArray<UChildActorComponent*> Children(D->Vehicle);
    for (auto* C : Children)
    {
        auto* A = C->GetChildActor();
        if (!A)
            continue;
        if (C->GetName().StartsWith(TEXT("GF")) && FinMaterial)
        {
            TInlineComponentArray<UStaticMeshComponent*> FinParts(A);
            for (auto* Part : FinParts)
                for (int I = 0; I < Part->GetNumMaterials(); ++I)
                    Part->SetMaterial(I, FinMaterial);
        }
        if (!C->GetName().StartsWith(TEXT("R")))
            continue;
        USceneComponent* Socket = nullptr;
        TInlineComponentArray<USceneComponent*> Parts(A);
        for (auto* Part : Parts)
        {
            if (Part->GetName() == TEXT("ThrustSocket"))
                Socket = Part;

        }
        if (!Socket)
            continue;
        auto* Plume = NewObject<UStaticMeshComponent>(GetOwner());
        Plume->SetStaticMesh(Mesh);
        auto* EngineMaterial = UMaterialInstanceDynamic::Create(Material, this);
        Plume->SetMaterial(0, EngineMaterial);
        PlumeMaterials.Add(EngineMaterial);
        ExhaustEnvelopes.Add(0);
        Plume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Plume->SetCastShadow(false);
        Plume->RegisterComponent();
        GetOwner()->AddInstanceComponent(Plume);
        Plumes.Add(Plume);
        NozzleCores->AddInstance(FTransform::Identity);
        EngineIndices.Add(
            D->GetEngines().IndexOfByPredicate([C](const FRecoveryEngineState& E) { return E.Id == C->GetFName(); }));
        // Small sources reveal the nozzle rim without lighting the entire site 33 times.
        auto* Light = NewObject<UPointLightComponent>(GetOwner(), FName(*(TEXT("ExhaustLight_") + C->GetName())));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Candelas);
        Light->SetUseInverseSquaredFalloff(true);
        Light->SetAttenuationRadius(1800);
        Light->SetSourceRadius(35);
        Light->SetSoftSourceRadius(65);
        // The distributed plume provides the scene shadow; nozzle sources stay local.
        Light->SetCastShadows(false);
        Light->SetVolumetricScatteringIntensity(0.f);
        Light->SetIntensity(0);
        Light->SetVisibility(false);
        Light->RegisterComponent();
        GetOwner()->AddInstanceComponent(Light);
        EngineLights.Add(Light);
    }
    // Broad, distributed light along the actual emitting column. Only the upper
    // source casts shadows; the small nozzle lights illuminate local metal only.
    for (int I = 0; I < 3; ++I)
    {
        auto* Light = NewObject<UPointLightComponent>(GetOwner(), FName(*FString::Printf(TEXT("PlumeLight_%d"), I)));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Candelas);
        Light->SetAttenuationRadius(30000);
        Light->SetSourceRadius(500);
        Light->SetSoftSourceRadius(900);
        Light->SetCastShadows(I == 0);
        Light->SetVolumetricScatteringIntensity(1.f);
        Light->SetLightColor(FLinearColor(1.f, .68f, .38f));
        Light->SetIntensity(0);
        Light->SetVisibility(false);
        Light->RegisterComponent();
        GetOwner()->AddInstanceComponent(Light);
        PlumeLights.Add(Light);
    }
    UpperStage = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("StarshipRenderBody"));
    UpperStage->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, RecoveryAssets::SM_StarshipDetailed));
    UpperStage->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UpperStage->RegisterComponent();
    GetOwner()->AddInstanceComponent(UpperStage);
    UpperStageFlameMaterial = UMaterialInstanceDynamic::Create(Material, this);
    for (int I = 0; I < 6; ++I)
    {
        auto* Plume = NewObject<UStaticMeshComponent>(GetOwner(), FName(*FString::Printf(TEXT("StarshipPlume_%d"), I)));
        Plume->SetStaticMesh(Mesh);
        Plume->SetMaterial(0, UpperStageFlameMaterial);
        Plume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Plume->SetCastShadow(false);
        Plume->SetVisibility(false);
        Plume->RegisterComponent();
        GetOwner()->AddInstanceComponent(Plume);
        UpperStagePlumes.Add(Plume);
    }
    if (auto* TrailSystem = LoadObject<UNiagaraSystem>(nullptr, RecoveryAssets::NS_RecoveryVaporTrail))
    {
        VaporTrail = NewObject<UNiagaraComponent>(GetOwner());
        VaporTrail->SetAutoActivate(false);
        VaporTrail->SetAsset(TrailSystem);
        VaporTrail->SetTickBehavior(ENiagaraTickBehavior::UsePrereqs);
        VaporTrail->PrimaryComponentTick.TickGroup = TG_LastDemotable;
        VaporTrail->AddTickPrerequisiteComponent(this);
        VaporTrail->SetCastShadow(false);
        VaporTrail->RegisterComponent();
        GetOwner()->AddInstanceComponent(VaporTrail);
        VaporTrail->Activate(true);
    }
    for (int I = 0; I < 6; ++I)
    {
        auto* Pod = NewObject<UStaticMeshComponent>(GetOwner());
        Pod->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, RecoveryAssets::SM_RCSBlock));
        Pod->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Pod->RegisterComponent();
        GetOwner()->AddInstanceComponent(Pod);
        RcsPods.Add(Pod);
        auto* Jet = NewObject<UStaticMeshComponent>(GetOwner());
        Jet->SetStaticMesh(Mesh);
        auto* MID = UMaterialInstanceDynamic::Create(
            LoadObject<UMaterialInterface>(nullptr, RecoveryAssets::M_AttitudeGas), this);
        Jet->SetMaterial(0, MID);
        Jet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Jet->SetCastShadow(false);
        Jet->RegisterComponent();
        GetOwner()->AddInstanceComponent(Jet);
        RcsPlumes.Add(Jet);
        RcsMaterials.Add(MID);
        RcsEnvelopes.Add(0);
        RcsDirections.Add(FVector::UpVector);
    }
    // Reuse the project's foliage and rock assets. Instances have no collision
    // and a bounded draw distance, so ground detail does not alter flight physics.
    const auto* Environment = LoadObject<URecoveryEnvironmentProfile>(nullptr, RecoveryAssets::DA_RecoveryEnvironment);
    for (int Kind = 0; Kind < 2; ++Kind)
    {
        auto* Detail = NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner());
        Detail->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Kind == 0 ? RecoveryAssets::SM_MWAM_GrassB
                                                                         : RecoveryAssets::SM_River_Rock));
        Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Detail->SetCastShadow(Kind == 1);
        if (Kind == 1)
            Detail->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, RecoveryAssets::M_RecoveryRock));
        Detail->SetCullDistances(Kind == 0 ? 20000 : 60000, Kind == 0 ? 65000 : 150000);
        Detail->RegisterComponent();
        GetOwner()->AddInstanceComponent(Detail);
        const TArray<FTransform> Instances =
            Environment ? (Kind == 0 ? Environment->Grass : Environment->Rocks) : TArray<FTransform>();
        Detail->AddInstances(Instances, false, true, false);
    }
    bBuilt = true;
}

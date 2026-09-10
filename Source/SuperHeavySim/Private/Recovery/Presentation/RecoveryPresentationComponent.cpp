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
#include "Components/PointLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

URecoveryPresentationComponent::URecoveryPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    // UWorld caches the player view after PostPhysics and BEFORE PostUpdateWork.
    // All vehicle artwork and the camera must use the same completed Chaos pose.
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void URecoveryPresentationComponent::Build()
{
    auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D || !D->Vehicle || !D->GetBody()) return;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,RecoveryAssets::SM_ExhaustEnvelope);
    auto* Material=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_RaptorPlume);
    if(!Mesh || !Material) return;
    MixingPlume=NewObject<UStaticMeshComponent>(GetOwner());
    MixingMaterial=UMaterialInstanceDynamic::Create(Material,this);
    MixingMaterial->SetScalarParameterValue(TEXT("MixingLayer"),1);
    MixingPlume->SetStaticMesh(Mesh);MixingPlume->SetMaterial(0,MixingMaterial);
    MixingPlume->SetCollisionEnabled(ECollisionEnabled::NoCollision);MixingPlume->SetCastShadow(false);
    MixingPlume->RegisterComponent();GetOwner()->AddInstanceComponent(MixingPlume);
    auto* FinMaterial=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_GridFinAlloy);
    if(auto* Steel=LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_BoosterFlight))
    {
        TInlineComponentArray<UStaticMeshComponent*> Hull(D->Vehicle);
        for(auto* Part:Hull)if(Part->GetName()==TEXT("SH_Body_Mesh"))Part->SetMaterial(0,Steel);
    }
    TInlineComponentArray<UChildActorComponent*> Children(D->Vehicle);
    for(auto* C:Children)
    {
        auto* A=C->GetChildActor(); if(!A) continue;
        if(C->GetName().StartsWith(TEXT("GF")) && FinMaterial)
        {
            TInlineComponentArray<UStaticMeshComponent*> FinParts(A);
            for(auto* Part:FinParts) for(int I=0;I<Part->GetNumMaterials();++I) Part->SetMaterial(I,FinMaterial);
        }
        if(!C->GetName().StartsWith(TEXT("R"))) continue;
        USceneComponent* Socket=nullptr;
        TInlineComponentArray<USceneComponent*> Parts(A);
        for(auto* Part:Parts)
        {
            if(Part->GetName()==TEXT("ThrustSocket")) Socket=Part;
            if(Part->GetName()==TEXT("ExhaustFX")) { Part->SetHiddenInGame(true); Part->SetVisibility(false,true); }
        }
        if(!Socket) continue;
        auto* Plume=NewObject<UStaticMeshComponent>(GetOwner());
        Plume->SetStaticMesh(Mesh);
        auto* EngineMaterial=UMaterialInstanceDynamic::Create(Material,this);
        Plume->SetMaterial(0,EngineMaterial);PlumeMaterials.Add(EngineMaterial);ExhaustEnvelopes.Add(0);
        Plume->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plume->SetCastShadow(false);
        Plume->RegisterComponent();GetOwner()->AddInstanceComponent(Plume);
        Plumes.Add(Plume);
        EngineIndices.Add(D->GetEngines().IndexOfByPredicate([C](const FRecoveryEngineState& E){return E.Id==C->GetFName();}));
        // Small sources reveal the nozzle rim without lighting the entire site 33 times.
        auto* Light=NewObject<UPointLightComponent>(GetOwner(),FName(*(TEXT("ExhaustLight_")+C->GetName())));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Candelas);Light->SetUseInverseSquaredFalloff(true);
        Light->SetAttenuationRadius(1800);Light->SetSourceRadius(35);Light->SetSoftSourceRadius(65);
        // The distributed plume provides the scene shadow; nozzle sources stay local.
        Light->SetCastShadows(false);
        Light->SetVolumetricScatteringIntensity(0.f);Light->SetIntensity(0);Light->SetVisibility(false);
        Light->RegisterComponent();GetOwner()->AddInstanceComponent(Light);EngineLights.Add(Light);
    }
    // Broad, distributed light along the actual emitting column. Only the upper
    // source casts shadows; the small nozzle lights illuminate local metal only.
    for(int I=0;I<3;++I)
    {
        auto* Light=NewObject<UPointLightComponent>(GetOwner(),FName(*FString::Printf(TEXT("PlumeLight_%d"),I)));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Candelas);
        Light->SetAttenuationRadius(30000);
        Light->SetSourceRadius(500);Light->SetSoftSourceRadius(900);
        Light->SetCastShadows(I==0);
        Light->SetVolumetricScatteringIntensity(1.f);
        Light->SetLightColor(FLinearColor(1.f,.68f,.38f));
        Light->SetIntensity(0);Light->SetVisibility(false);
        Light->RegisterComponent();GetOwner()->AddInstanceComponent(Light);PlumeLights.Add(Light);
    }
    UpperStage=NewObject<UStaticMeshComponent>(GetOwner(),TEXT("StarshipRenderBody"));
    UpperStage->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,RecoveryAssets::SM_StarshipDetailed));
    UpperStage->SetCollisionEnabled(ECollisionEnabled::NoCollision);UpperStage->RegisterComponent();GetOwner()->AddInstanceComponent(UpperStage);
    UpperStageFlameMaterial=UMaterialInstanceDynamic::Create(Material,this);
    for(int I=0;I<6;++I)
    {
        auto* Plume=NewObject<UStaticMeshComponent>(GetOwner(),FName(*FString::Printf(TEXT("StarshipPlume_%d"),I)));
        Plume->SetStaticMesh(Mesh);Plume->SetMaterial(0,UpperStageFlameMaterial);
        Plume->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plume->SetCastShadow(false);
        Plume->SetVisibility(false);Plume->RegisterComponent();GetOwner()->AddInstanceComponent(Plume);
        UpperStagePlumes.Add(Plume);
    }
    if(auto* TrailSystem=LoadObject<UNiagaraSystem>(nullptr,RecoveryAssets::NS_RecoveryVaporTrail))
    {
        VaporTrail=NewObject<UNiagaraComponent>(GetOwner());
        VaporTrail->SetAutoActivate(false);VaporTrail->SetAsset(TrailSystem);
        VaporTrail->SetTickBehavior(ENiagaraTickBehavior::UsePrereqs);
        VaporTrail->PrimaryComponentTick.TickGroup=TG_LastDemotable;
        VaporTrail->AddTickPrerequisiteComponent(this);
        VaporTrail->SetCastShadow(false);VaporTrail->RegisterComponent();
        GetOwner()->AddInstanceComponent(VaporTrail);VaporTrail->Activate(true);
    }
    for(int I=0;I<6;++I)
    {
        auto* Pod=NewObject<UStaticMeshComponent>(GetOwner());Pod->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,RecoveryAssets::SM_RCSBlock));
        Pod->SetCollisionEnabled(ECollisionEnabled::NoCollision);Pod->RegisterComponent();GetOwner()->AddInstanceComponent(Pod);RcsPods.Add(Pod);
        auto* Jet=NewObject<UStaticMeshComponent>(GetOwner());Jet->SetStaticMesh(Mesh);
        auto* MID=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_AttitudeGas),this);
        Jet->SetMaterial(0,MID);Jet->SetCollisionEnabled(ECollisionEnabled::NoCollision);Jet->SetCastShadow(false);
        Jet->RegisterComponent();GetOwner()->AddInstanceComponent(Jet);RcsPlumes.Add(Jet);RcsMaterials.Add(MID);
        RcsEnvelopes.Add(0);RcsDirections.Add(FVector::UpVector);
    }
    // Reuse the project's foliage and rock assets. Instances have no collision
    // and a bounded draw distance, so ground detail does not alter flight physics.
    const auto* Environment=LoadObject<URecoveryEnvironmentProfile>(nullptr,RecoveryAssets::DA_RecoveryEnvironment);
    for(int Kind=0;Kind<2;++Kind)
    {
        auto* Detail=NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner());
        Detail->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Kind==0 ?
            RecoveryAssets::SM_MWAM_GrassB :
            RecoveryAssets::SM_River_Rock));
        Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);Detail->SetCastShadow(Kind==1);
        if(Kind==1) Detail->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,RecoveryAssets::M_RecoveryRock));
        Detail->SetCullDistances(Kind==0?20000:60000,Kind==0?65000:150000);
        Detail->RegisterComponent();GetOwner()->AddInstanceComponent(Detail);
        const TArray<FTransform> Instances=Environment?(Kind==0?Environment->Grass:Environment->Rocks):TArray<FTransform>();
        Detail->AddInstances(Instances,false,true,false);
    }
    bBuilt=true;
}
void URecoveryPresentationComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(!URecoveryStartupSubsystem::AssetsLoaded(GetWorld()))return;
    if(!FApp::CanEverRender()) { SetComponentTickEnabled(false); return; }
    auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());if(!D || !D->GetBody()) return;
    if(!bBuilt) Build();if(!bBuilt) return;
    if(LastGeneration!=D->GetMissionGeneration())
    {
        for(double& E:ExhaustEnvelopes)E=0;
        for(double& E:RcsEnvelopes)E=0;
        LastGeneration=D->GetMissionGeneration();
    }
    SyncActuatorMeshes();
    Clock+=Dt;
    const auto* P=D->GetProfile(); const double Vacuum=1-FMath::Sqrt(FMath::Clamp(D->PressurePa/101325.,0.,1.));
    const auto* UI=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    const double LightScale=UI?UI->EngineLightScale:1.;
    int LitEngines=0,BurningEngines=0;
    double DeliveredPower=0;
    for(int I=0;I<Plumes.Num();++I)
    {
        if(!D->GetEngines().IsValidIndex(EngineIndices[I]))continue;
        const auto& Engine=D->GetEngines()[EngineIndices[I]];
        const double Delivered=FMath::Clamp(Engine.ThrustN/(P->EngineThrustN*D->EngineIspS/P->SpecificImpulseSeaLevelS),0.,1.);
        ExhaustEnvelopes[I]=RecoveryPropulsionVisuals::ExhaustEnvelope(ExhaustEnvelopes[I],Delivered,Dt);
        const double Power=ExhaustEnvelopes[I];
        PlumeMaterials[I]->SetScalarParameterValue(TEXT("Throttle"),Power);
        PlumeMaterials[I]->SetScalarParameterValue(TEXT("Vacuum"),Vacuum);
        PlumeMaterials[I]->SetScalarParameterValue(TEXT("FlightTime"),Clock+I*.137);
        DeliveredPower+=Power;
        const bool On=Power>.01;
        if(On)++BurningEngines;
        const FQuat BodyQ=D->GetBody()->GetComponentQuat();
        const FQuat Gimbal=FQuat::FindBetweenNormals(FVector::UpVector,Engine.DirectionBody);
        const FVector Nozzle=FlightGeometry::BoosterBaseCm(*D->GetBody())+BodyQ.RotateVector(Engine.PositionFromBaseM+Gimbal.RotateVector(Engine.NozzleOffsetBodyM))*100;
        const FQuat Orientation=BodyQ*Gimbal;
        Plumes[I]->SetVisibility(On);
        Plumes[I]->SetWorldLocationAndRotation(Nozzle,Orientation);
        const double Flicker=1+0.035*FMath::Sin(Clock*41+I*2.17)+0.02*FMath::Sin(Clock*73+I);
        Plumes[I]->SetWorldScale3D(FVector(1.35+Vacuum*4,1.35+Vacuum*4,(20+30*Power)*(1+Vacuum*0.9)*Flicker));
        EngineLights[I]->SetWorldLocation(Nozzle-Orientation.GetUpVector()*250);
        EngineLights[I]->SetLightColor(FMath::Lerp(FLinearColor(0.48f,0.67f,1.f),FLinearColor(1.f,0.48f,0.2f),float(Power*0.8)));
        EngineLights[I]->SetVisibility(On && LightScale>0);
        EngineLights[I]->SetIntensity(On?12000.*Power*Flicker*LightScale:0.);
        if(EngineLights[I]->IsVisible()) ++LitEngines;
    }
    if(LitEngines!=LastLitEngineCount)
    {
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_LIGHTS configured=%d expected=%d active=%d throttle=%.3f scale=%.1f"),EngineLights.Num(),D->ActiveEngines,LitEngines,D->Throttle,LightScale);
        LastLitEngineCount=LitEngines;
    }
    // Use one post-physics rigid-body pose for every attached visual. Mixing the
    // navigation sample with the new body rotation produced visible stage drift.
    const FVector Up=D->GetBody()->GetUpVector();
    const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody());
    const double MeanPower=FMath::Clamp(DeliveredPower/FMath::Max(1,BurningEngines),0.,1.);
    for(int I=0;I<PlumeLights.Num();++I)
    {
        PlumeLights[I]->SetWorldLocation(Base-Up*(1000+I*1500));
        PlumeLights[I]->SetIntensity(120000.*(DeliveredPower/33.)*FMath::Sqrt(MeanPower)*LightScale/(1+I*.5));
        PlumeLights[I]->SetVisibility(DeliveredPower>.01 && LightScale>0);
    }
    MixingPlume->SetWorldLocationAndRotation(Base-Up*900,D->GetBody()->GetComponentQuat());
    const double EngineScale=FMath::Sqrt(BurningEngines/33.);
    MixingPlume->SetWorldScale3D(FVector((10+Vacuum*18)*EngineScale,(10+Vacuum*18)*EngineScale,(40+45*MeanPower)*(0.5+0.5*EngineScale)));
    MixingPlume->SetVisibility(DeliveredPower>.01);
    MixingMaterial->SetScalarParameterValue(TEXT("Throttle"),MeanPower);
    MixingMaterial->SetScalarParameterValue(TEXT("Vacuum"),Vacuum);
    MixingMaterial->SetScalarParameterValue(TEXT("FlightTime"),Clock);
    if(VaporTrail)
    {
        const bool Reset=D->Phase==ERecoveryPhase::Ready || D->Phase==ERecoveryPhase::Countdown;
        if(Reset && bTrailWasRunning) { VaporTrail->ReinitializeSystem();bTrailWasRunning=false; }
        const double Air=FMath::Clamp((D->PressurePa/101325.-0.015)/0.985,0.,1.);
        const double Power=RecoveryPropulsionVisuals::DeliveredFraction(D->GetEngines(),P->EngineThrustN);
        // Interpolated GPU spawning fills the path between frames. Old particles
        // remain in world space and drift with wind after engine shutdown.
        const double Rate=D->Phase==ERecoveryPhase::Ascent?FMath::Clamp(70+D->VelocityMps.Size()*0.55,70.,330.)*FMath::Sqrt(Air)*Power:0;
        const double TailM=FMath::Min(FMath::Max(D->AltitudeM-6.,0.),35.+20.*D->Throttle);
        VaporTrail->SetWorldLocation(Base-Up*TailM*100);
        VaporTrail->SetVariableFloat(TEXT("SpawnRate"),Rate);
        const FVector Wind=P?P->WindVelocityMps*100:FVector::ZeroVector;
        VaporTrail->SetVariableVec3(TEXT("Wind"),Wind);
        VaporTrail->SetVariableVec3(TEXT("ExhaustVelocity"),Wind-Up*(800+1200*Power));
        if(Rate>0.01) bTrailWasRunning=true;
    }
    const FTransform ShipPose=D->GetUpperStageBaseTransform();
    UpperStage->SetWorldLocationAndRotation(ShipPose.GetLocation(),ShipPose.GetRotation());
    UpperStage->SetVisibility(P && P->UpperStageMassKg>0 && (!D->bSeparated || (ShipPose.GetLocation()-Base).Size()<4000000));
    const double ShipPower=FMath::Clamp(D->GetUpperStageThrustN()/(6*P->UpperStageEngineThrustN),0.,1.);
    // The separated stage owns its pressure sample and rigid-body pose.
    const double ShipVacuum=1-FMath::Sqrt(FMath::Clamp(RecoveryAtmosphere::Sample(FlightGeometry::AltitudeM(ShipPose.GetLocation()),P->SeaLevelTemperatureOffsetK).Pressure/101325.,0.,1.));
    UpperStageFlameMaterial->SetScalarParameterValue(TEXT("Throttle"),ShipPower);
    UpperStageFlameMaterial->SetScalarParameterValue(TEXT("Vacuum"),ShipVacuum);
    UpperStageFlameMaterial->SetScalarParameterValue(TEXT("FlightTime"),Clock);
    for(int I=0;I<UpperStagePlumes.Num();++I)
    {
        UpperStagePlumes[I]->SetVisibility(UpperStage->IsVisible() && ShipPower>.01);
        UpperStagePlumes[I]->SetWorldLocationAndRotation(ShipPose.TransformPosition(FlightGeometry::UpperStageNozzlePositionsM()[I]*100),ShipPose.GetRotation());
        const double Radius=(I<3?1.4:2.2)*(1+ShipVacuum*2.5);
        UpperStagePlumes[I]->SetWorldScale3D(FVector(Radius,Radius,(20+30*ShipPower)*(1+ShipVacuum)));
    }
    const auto& Positions=FlightGeometry::ReactionNozzlePositionsM();
    const auto& Forces=D->GetReactionForcesBodyN();
    for(int I=0;I<RcsPods.Num();++I)
    {
        const FQuat Q=D->GetBody()->GetComponentQuat();const FVector Position=Base+Q.RotateVector(Positions[I])*100;
        RcsPods[I]->SetWorldLocationAndRotation(Position,Q*FRotationMatrix::MakeFromX(FVector(Positions[I].X,Positions[I].Y,0)).ToQuat());
        const double ForceN=Forces[I].Size();
        if(ForceN>=1)RcsDirections[I]=Forces[I]/ForceN;
        RcsEnvelopes[I]=RecoveryPropulsionVisuals::ReactionEnvelope(RcsEnvelopes[I],ForceN,Dt);
        const double Power=RcsEnvelopes[I];
        RcsPlumes[I]->SetVisibility(Power>0);
        // The envelope extends along local -Z, opposite the measured force.
        RcsPlumes[I]->SetWorldLocationAndRotation(Position,FRotationMatrix::MakeFromZ(Q.RotateVector(RcsDirections[I])).ToQuat());
        const double Width=(.16+Power*.48)*(1+Vacuum*.85);
        RcsPlumes[I]->SetWorldScale3D(FVector(Width,Width,(2.5+Power*9)*(1+Vacuum*.5)));
        RcsMaterials[I]->SetScalarParameterValue(TEXT("Power"),Power);RcsMaterials[I]->SetScalarParameterValue(TEXT("Time"),Clock);
    }
    D->UpdateCamera(Dt);
}

void URecoveryPresentationComponent::SyncActuatorMeshes()
{
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D || !D->Vehicle || !D->GetProfile())return;
    auto* Vehicle=D->Vehicle.Get();
    Vehicle->SetGridFinAngleCommand(TEXT("GF_XP"),D->GridFinAnglesDeg.X);
    Vehicle->SetGridFinAngleCommand(TEXT("GF_XM"),D->GridFinAnglesDeg.Y);
    Vehicle->SetGridFinAngleCommand(TEXT("GF_YM"),D->GridFinAnglesDeg.Z);
    const double PerEngine=D->GetProfile()->EngineThrustN*D->EngineIspS/D->GetProfile()->SpecificImpulseSeaLevelS;
    for(const auto& Engine:D->GetEngines())
    {
        Vehicle->SetEngineThrottleCommand(Engine.Id,FMath::Clamp(Engine.ThrustN/PerEngine,0.,1.));
        if(Engine.bGimballed)Vehicle->SetEngineGimbalCommand(Engine.Id,
            FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.X,Engine.DirectionBody.Z)),
            FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.Y,Engine.DirectionBody.Z)));
    }
}

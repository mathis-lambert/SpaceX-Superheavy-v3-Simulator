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

URecoveryPresentationComponent::URecoveryPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    // UWorld caches the player view after PostPhysics and BEFORE PostUpdateWork.
    // All vehicle artwork and the camera must use the same completed Chaos pose.
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
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
        const double Delivered=FMath::Clamp(Engine.ThrustN/(P->EngineThrustN*D->GetEngineSpecificImpulseS()/P->SpecificImpulseSeaLevelS),0.,1.);
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
        Plumes[I]->SetWorldScale3D(FVector(1.,1.,(20+30*Power)*(1+Vacuum*0.9)*Flicker));
        NozzleCores->UpdateInstanceTransform(I,FTransform(Orientation,Nozzle+Orientation.GetUpVector()*12,On?FVector(.88,.88,.025):FVector::ZeroVector),true,false,true);
        NozzleCores->SetCustomDataValue(I,0,Power*Flicker,false);
        EngineLights[I]->SetWorldLocation(Nozzle+Orientation.GetUpVector()*35);
        EngineLights[I]->SetLightColor(FMath::Lerp(FLinearColor(0.48f,0.67f,1.f),FLinearColor(1.f,0.48f,0.2f),float(Power*0.8)));
        EngineLights[I]->SetVisibility(On && LightScale>0);
        EngineLights[I]->SetIntensity(On?12000.*Power*Flicker*LightScale:0.);
        if(EngineLights[I]->IsVisible()) ++LitEngines;
    }
    NozzleCores->MarkRenderStateDirty();
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
    MixingPlume->SetWorldScale3D(FVector((6+Vacuum*10)*EngineScale,(6+Vacuum*10)*EngineScale,(40+45*MeanPower)*(0.5+0.5*EngineScale)));
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
        const bool AtmosphericBurn=D->Phase==ERecoveryPhase::Ascent || D->Phase==ERecoveryPhase::LandingBurn;
        const double Rate=AtmosphericBurn?FMath::Clamp(70+D->VelocityMps.Size()*0.55,70.,330.)*FMath::Sqrt(Air)*Power:0;
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
}

void URecoveryPresentationComponent::SyncActuatorMeshes()
{
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D || !D->Vehicle || !D->GetProfile())return;
    auto* Vehicle=D->Vehicle.Get();
    Vehicle->SetGridFinAngleCommand(TEXT("GF_XP"),D->GridFinAnglesDeg.X);
    Vehicle->SetGridFinAngleCommand(TEXT("GF_XM"),D->GridFinAnglesDeg.Y);
    Vehicle->SetGridFinAngleCommand(TEXT("GF_YM"),D->GridFinAnglesDeg.Z);
    const double PerEngine=D->GetProfile()->EngineThrustN*D->GetEngineSpecificImpulseS()/D->GetProfile()->SpecificImpulseSeaLevelS;
    for(const auto& Engine:D->GetEngines())
    {
        Vehicle->SetEngineThrottleCommand(Engine.Id,FMath::Clamp(Engine.ThrustN/PerEngine,0.,1.));
        if(Engine.bGimballed)Vehicle->SetEngineGimbalCommand(Engine.Id,
            FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.X,Engine.DirectionBody.Z)),
            FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.Y,Engine.DirectionBody.Z)));
    }
}

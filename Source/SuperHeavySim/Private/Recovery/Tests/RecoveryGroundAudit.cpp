#include "Recovery/Tests/RecoveryDiagnosticsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Components/HeterogeneousVolumeComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "HAL/IConsoleManager.h"

void URecoveryDiagnosticsComponent::TickGroundAudit(float Dt)
{
    auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    auto* PC=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    if(!D || !PC || !D->GetBody())return;
    GroundAuditClock+=Dt;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery/GroundAudit");
    IFileManager::Get().MakeDirectory(*Dir,true);
    const auto Check=[&](const TCHAR* Label,bool Pass)
    {bGroundPassed&=Pass;GroundChecks.Add(FString(Label)+(Pass?TEXT(": PASS"):TEXT(": FAIL")));UE_LOG(LogTemp,Display,TEXT("GROUND_CHECK %s %d"),Label,Pass);};
    const auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/Name,true,false);};
    const auto* Vapor=D->FindComponentByClass<URecoveryVaporComponent>();
    const double Remaining=D->GetLaunchSequence().RemainingS;
    if(GroundStage==0 && GroundAuditClock>8)
    {
        Check(TEXT("Home remains a live conditioned vehicle"),PC->IsAtHome() && D->Phase==ERecoveryPhase::Ready && D->GetConditioningFlowKgS().Size()>.1);
        Check(TEXT("Both home condensation volumes are registered and visible"),Vapor && Vapor->GetCryogenicVolumeCount()==2);
        TInlineComponentArray<UHeterogeneousVolumeComponent*> Volumes(D);
        for(const auto* V:Volumes)
        {
            const auto* TraceDistance=IConsoleManager::Get().FindConsoleVariable(TEXT("r.HeterogeneousVolumes.MaxTraceDistance"));
            Check(TEXT("Home camera can trace through the complete condensation volume"),PC->GetViewTarget() && TraceDistance && TraceDistance->GetFloat()>(PC->GetViewTarget()->GetActorLocation()-V->Bounds.Origin).Size()+V->Bounds.SphereRadius);
            auto* M=Cast<UMaterialInstanceDynamic>(V->GetMaterial(0));
            UE_LOG(LogTemp,Display,TEXT("GROUND_VOLUME %s visible=%d bounds=%s center=%s strength=%.3f vent=%s resolution=%s"),*V->GetName(),V->IsVisible(),*V->Bounds.BoxExtent.ToString(),*V->Bounds.Origin.ToString(),M?M->K2_GetScalarParameterValue(TEXT("FlowStrength")):-1,M?*M->K2_GetVectorParameterValue(TEXT("VentPosition")).ToString():TEXT("none"),*V->VolumeResolution.ToString());
        }
        GroundGeneration=D->GetMissionGeneration();Shot(TEXT("Home.png"));GroundStage=1;
    }
    else if(GroundStage==1 && GroundAuditClock>11)
    {PC->StartingCamera=0;PC->bTelemetry=true;PC->LaunchFlight();GroundStage=2;}
    else if(GroundStage==2 && Remaining<55)
    {
        Check(TEXT("Launch preserves the conditioned vehicle"),D->GetMissionGeneration()==GroundGeneration);
        Check(TEXT("Signed clock counts down with engines off and mount attached"),D->Phase==ERecoveryPhase::Countdown && D->MissionTime<0 && D->ActiveEngines==0 && !D->IsLaunchMountReleased());
        Shot(TEXT("TerminalCount.png"));D->SetFailedEngine(0);GroundHoldTime=D->MissionTime;GroundAuditClock=0;GroundStage=3;
    }
    else if(GroundStage==3 && GroundAuditClock>3)
    {
        Check(TEXT("Real engine failure holds the cold countdown"),D->GetLaunchSequence().bHeld && D->MissionTime==GroundHoldTime && !D->IsLaunchMountReleased());
        Shot(TEXT("Hold.png"));D->ResetExperiments();GroundStage=4;
    }
    else if(GroundStage==4 && Remaining<8)
    {
        Check(TEXT("Water flows before engine ignition"),D->GetDelugeFlow()>.9 && D->ActiveEngines==0 && !D->IsLaunchMountReleased());
        Shot(TEXT("Deluge.png"));GroundStage=5;
    }
    else if(GroundStage==5 && Remaining<1 && D->Phase==ERecoveryPhase::Countdown)
    {
        Check(TEXT("Engines build physical thrust while the mount is attached"),D->Throttle>.9 && D->ActiveEngines==33 && !D->IsLaunchMountReleased());
        Shot(TEXT("Ignition.png"));GroundStage=6;
    }
    else if(GroundStage==6 && D->Phase==ERecoveryPhase::Ascent && D->MissionTime>6)
    {
        Check(TEXT("Verified thrust releases the mount and produces ascent"),D->IsLaunchMountReleased() && D->VerticalSpeedMps>1 && D->MissionTime>=0);
        Shot(TEXT("Liftoff.png"));GroundStage=7;GroundAuditClock=0;
    }
    else if(GroundStage==7 && GroundAuditClock>2)
    {
        auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),bGroundPassed);
        TArray<TSharedPtr<FJsonValue>> Checks;for(const auto& C:GroundChecks)Checks.Add(MakeShared<FJsonValueString>(C));R->SetArrayField(TEXT("checks"),Checks);
        FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("result.json")));
        FPlatformMisc::RequestExitWithStatus(false,bGroundPassed?0:1);
    }
}

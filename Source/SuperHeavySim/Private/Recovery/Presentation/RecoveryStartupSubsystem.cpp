#include "Recovery/Presentation/RecoveryStartupSubsystem.h"
#include "RecoveryLoadingScreen.h"
#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Presentation/RecoveryPresentationComponent.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Presentation/RecoverySkyComponent.h"
#include "Recovery/Presentation/RecoverySiteDetailsComponent.h"
#include "Recovery/Presentation/RecoverySiteActivityComponent.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Shared/RecoveryLog.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "ContentStreaming.h"
#include "ShaderPipelineCache.h"
#include "ShaderCompiler.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Serialization/JsonSerializer.h"

bool URecoveryStartupSubsystem::IsReady(const UWorld* World)
{
    const auto* Startup=World?World->GetSubsystem<URecoveryStartupSubsystem>():nullptr;
    return !Startup || !Startup->bEnabled || (Startup->bReady && !Startup->Screen);
}
bool URecoveryStartupSubsystem::AssetsLoaded(const UWorld* World)
{
    const auto* Startup=World?World->GetSubsystem<URecoveryStartupSubsystem>():nullptr;
    return !Startup || !Startup->bEnabled || Startup->bAssetsLoaded;
}
void URecoveryStartupSubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    bEnabled=FApp::CanEverRender() && !IsRunningCommandlet();
    if(!bEnabled)return;
    bPreviousScreenMessages=GAreScreenMessagesEnabled;GAreScreenMessagesEnabled=false;
    Started=FPlatformTime::Seconds();Status=FText::FromString(TEXT("LOADING RESOURCES"));
    bStartupAudit=FParse::Param(FCommandLine::Get(),TEXT("RecoveryStartupAudit"));
    Screen=SNew(SRecoveryLoadingScreen).Status_Lambda([this](){return Status;})
        .Progress_Lambda([this](){return Progress;});
    AssetHandle=UAssetManager::GetStreamableManager().RequestAsyncLoad(RecoveryAssets::StartupAssets());
    UE_LOG(LogRecovery,Display,TEXT("RECOVERY_STARTUP loading resources"));
}
void URecoveryStartupSubsystem::Tick(float DeltaTime)
{
    if(AuditExitAt>0)
    {
        if(!bAuditHomeShot && FPlatformTime::Seconds()>AuditExitAt-1)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Recovery/Startup/Home.png"),true,false);
            bAuditHomeShot=true;
        }
        if(FPlatformTime::Seconds()>AuditExitAt)FPlatformMisc::RequestExit(false);
    }
    if(!bEnabled || !Screen)return;
    // Attach when the game viewport is available, on
    // the first world tick, while the early engine screen hands over rendering.
    if(!bViewportAttached && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->AddViewportWidgetContent(Screen.ToSharedRef(),10000);bViewportAttached=true;
    }
    ++FrameCount;
    if(bStartupAudit && FrameCount==3)
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Recovery/Startup/Loading.png"),true,false);
    if(bReady)
    {
        const double Fade=FMath::Clamp((FPlatformTime::Seconds()-ReadyAt)/.45,0.,1.);
        Screen->SetRenderOpacity(1-Fade);
        if(Fade>=1)
        {
            if(GEngine && GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
            Screen.Reset();
            GAreScreenMessagesEnabled=bPreviousScreenMessages;
            if(bStartupAudit)AuditExitAt=FPlatformTime::Seconds()+2;
            UE_LOG(LogRecovery,Display,TEXT("RECOVERY_STARTUP reveal complete"));
        }
        return;
    }
    if(!AssetHandle){Status=FText::FromString(TEXT("RESOURCE LOAD FAILED"));bFailed=true;return;}
    AssetHandle->GetLoadedCount(Loaded,Requested);
    if(!AssetHandle->HasLoadCompleted())
    {
        Progress=AssetHandle->GetProgress();
        Status=FText::FromString(FString::Printf(TEXT("RESOURCES  /  %d OF %d"),Loaded,Requested));return;
    }
    if(!bAssetsLoaded)
    {
        TArray<UObject*> Assets;AssetHandle->GetLoadedAssets(Assets);
        for(auto* Asset:Assets)if(Asset)RetainedAssets.Add(Asset);
        if(RetainedAssets.Num()!=Requested)
        {
            bFailed=true;Status=FText::FromString(TEXT("MISSING RESOURCES / SEE STARTUP REPORT"));
            WriteReport(0,0);return;
        }
        bAssetsLoaded=true;
    }
    ASuperHeavyRecoveryDirector* Director=nullptr;
    for(TActorIterator<ASuperHeavyRecoveryDirector> It(GetWorld());It;++It){Director=*It;break;}
    if(!Director || !Director->GetBody())return;
    if(ARecoveryPlayerController::ShouldShowFrontend())Director->Viewer->bFrontendView=true;
    const auto* Presentation=Director->FindComponentByClass<URecoveryPresentationComponent>();
    const auto* Vapor=Director->FindComponentByClass<URecoveryVaporComponent>();
    const auto* Audio=Director->FindComponentByClass<URecoveryAudioComponent>();
    const auto* Sky=Director->FindComponentByClass<URecoverySkyComponent>();
    const auto* Details=Director->FindComponentByClass<URecoverySiteDetailsComponent>();
    const auto* Activity=Director->FindComponentByClass<URecoverySiteActivityComponent>();
    const bool Built=Presentation && Presentation->IsReady() && Vapor && Vapor->IsReady() && Audio && Audio->IsReady() &&
        Sky && Sky->IsReady() && Details && Details->IsReady() && Activity && Activity->IsReady();
    if(!Built){Status=FText::FromString(TEXT("PREPARING STARBASE"));Progress.Reset();return;}
    if(!bPrecached)
    {
        // Include hidden plumes and the bounded volume pool: their PSOs should
        // compile here instead of on the first engine ignition.
        for(TActorIterator<AActor> It(GetWorld());It;++It)
        {
            TInlineComponentArray<UPrimitiveComponent*> Components(*It);
            for(auto* Component:Components)Component->PrecachePSOs();
        }
        IStreamingManager::Get().NotifyLevelChange();
        bPrecached=true;
    }
    const int PSOs=FShaderPipelineCache::NumPrecompilesRemaining();
    const int ShaderJobs=GShaderCompilingManager?GShaderCompilingManager->GetNumRemainingJobs():0;
    const int Textures=IStreamingManager::Get().GetNumWantingResources();
    InitialPSOs=FMath::Max(InitialPSOs,PSOs);InitialTextures=FMath::Max(InitialTextures,Textures);
    if(ShaderJobs>0)
    {
        Status=FText::FromString(FString::Printf(TEXT("PREPARING SHADERS  /  %d REMAINING"),ShaderJobs));Progress.Reset();
    }
    else if(PSOs>0)
    {
        Status=FText::FromString(FString::Printf(TEXT("PREPARING RENDERER  /  %d REMAINING"),PSOs));
        Progress=1-float(PSOs)/FMath::Max(1,InitialPSOs);
    }
    else if(Textures>0)
    {
        Status=FText::FromString(FString::Printf(TEXT("STREAMING TEXTURES  /  %d REMAINING"),Textures));
        Progress=1-float(Textures)/FMath::Max(1,InitialTextures);
    }
    else {Status=FText::FromString(TEXT("PREPARING VIEW"));Progress.Reset();}
    // Several real rendered frames allow streaming queries and newly enqueued
    // component/global PSOs to settle. No time-based fake loading percentage.
    QuietFrames=ShaderJobs==0 && PSOs==0 && Textures==0?QuietFrames+1:0;
    if(QuietFrames>=12 && FrameCount>=16)
    {
        bReady=true;ReadyAt=FPlatformTime::Seconds();Progress=1;Status=FText::FromString(TEXT("READY"));
        WriteReport(PSOs,Textures);
        UE_LOG(LogRecovery,Display,TEXT("RECOVERY_STARTUP ready seconds=%.3f assets=%d peak_psos=%d peak_textures=%d frames=%d"),ReadyAt-Started,Loaded,InitialPSOs,InitialTextures,FrameCount);
    }
}
void URecoveryStartupSubsystem::WriteReport(int PSOs,int Textures)
{
    auto Report=MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("ready"),bReady);Report->SetBoolField(TEXT("failed"),bFailed);
    Report->SetNumberField(TEXT("engine_elapsed_seconds"),FPlatformTime::Seconds()-GStartTime);
    Report->SetNumberField(TEXT("scene_prepare_seconds"),FPlatformTime::Seconds()-Started);
    Report->SetNumberField(TEXT("assets_loaded"),RetainedAssets.Num());Report->SetNumberField(TEXT("assets_requested"),Requested);
    Report->SetNumberField(TEXT("pending_psos_at_reveal"),PSOs);Report->SetNumberField(TEXT("pending_textures_at_reveal"),Textures);
    Report->SetNumberField(TEXT("pending_shaders_at_reveal"),GShaderCompilingManager?GShaderCompilingManager->GetNumRemainingJobs():0);
    Report->SetNumberField(TEXT("peak_pending_psos"),InitialPSOs);Report->SetNumberField(TEXT("peak_pending_textures"),InitialTextures);
    Report->SetNumberField(TEXT("rendered_frames"),FrameCount);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Recovery")),true);
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("Recovery/startup-report.json")));
}
void URecoveryStartupSubsystem::Deinitialize()
{
    if(bEnabled && Screen)GAreScreenMessagesEnabled=bPreviousScreenMessages;
    if(Screen && GEngine && GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
    Screen.Reset();if(AssetHandle)AssetHandle->CancelHandle();AssetHandle.Reset();RetainedAssets.Reset();
    Super::Deinitialize();
}

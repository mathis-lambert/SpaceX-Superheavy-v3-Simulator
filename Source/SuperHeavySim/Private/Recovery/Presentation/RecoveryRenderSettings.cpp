#include "Recovery/Presentation/RecoveryRenderSettings.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/App.h"
#include "RenderUtils.h"
#if RECOVERY_WITH_DLSS
#include "DLSSLibrary.h"
#endif

namespace RecoveryRenderSettings
{
    bool SupportsHardwareRayTracing()
    { return FApp::CanEverRender() && IsRayTracingEnabled(); }
    bool ApplyHardwareRayTracing(bool Requested)
    {
        const bool Enabled=Requested && SupportsHardwareRayTracing();
        if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing")))C->Set(Enabled?1:0,ECVF_SetByCode);
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_RAY_TRACING requested=%d enabled=%d supported=%d"),Requested,Enabled,SupportsHardwareRayTracing());
        return Enabled;
    }
    bool SupportsDLSS()
    {
#if RECOVERY_WITH_DLSS
        return FApp::CanEverRender() && UDLSSLibrary::IsDLSSSupported();
#else
        return false;
#endif
    }
    TArray<FString> ReconstructionNames()
    {
        TArray<FString> Names={TEXT("Native / TSR"),TEXT("TSR Quality / 75%")};
        if(SupportsDLSS())Names.Append({TEXT("NVIDIA DLAA / native"),TEXT("NVIDIA DLSS / Quality"),TEXT("NVIDIA DLSS / Balanced")});
        return Names;
    }
    int32 ApplyReconstruction(int32 RequestedMode)
    {
        if(!FApp::CanEverRender())return 0;
        int32 Mode=FMath::Clamp(RequestedMode,0,4);
        if(Mode>=2 && !SupportsDLSS())Mode=0;
        float ScreenPercentage=Mode==1?75.f:100.f;
#if RECOVERY_WITH_DLSS
        if(Mode>=2)
        {
            bool Supported=false,Fixed=false;float Minimum=0,Maximum=100,Sharpness=0;
            const UDLSSMode Quality=Mode==2?UDLSSMode::DLAA:Mode==3?UDLSSMode::Quality:UDLSSMode::Balanced;
            const FIntPoint Resolution=GEngine->GetGameUserSettings()->GetScreenResolution();
            UDLSSLibrary::GetDLSSModeInformation(Quality,FVector2D(Resolution),Supported,ScreenPercentage,Fixed,Minimum,Maximum,Sharpness);
            if(!Supported){Mode=0;ScreenPercentage=100;}
        }
        UDLSSLibrary::EnableDLSS(Mode>=2);
#endif
        if(auto* AA=IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))AA->Set(4,ECVF_SetByCode);
        if(auto* Percentage=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))Percentage->Set(ScreenPercentage,ECVF_SetByCode);
        // Preserve approximately the same fog-grid footprint at the output
        // resolution when reconstructing a smaller scene image. Otherwise
        // DLSS also silently makes each vapor froxel much larger on screen.
        const int32 ShadowQuality=GEngine->GetGameUserSettings()->GetShadowQuality();
        const int32 NativeFogPixelSize=ShadowQuality>=4?4:ShadowQuality>=3?8:16;
        const int32 FogPixelSize=FMath::Max(4,FMath::RoundToInt(NativeFogPixelSize*ScreenPercentage/100.f));
        if(auto* Fog=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.GridPixelSize")))Fog->Set(FogPixelSize,ECVF_SetByCode);
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_RECONSTRUCTION mode=%d screen_percentage=%.3f dlss_supported=%d"),Mode,ScreenPercentage,SupportsDLSS());
        UE_LOG(LogTemp,Display,TEXT("RECOVERY_FOG_GRID pixels=%d native_reference=%d"),FogPixelSize,NativeFogPixelSize);
        return Mode;
    }
    FString ActiveReconstruction()
    {
#if RECOVERY_WITH_DLSS
        if(FApp::CanEverRender() && UDLSSLibrary::IsDLSSEnabled())return TEXT("NVIDIA DLSS / DLAA");
#endif
        return TEXT("Unreal TSR");
    }
}

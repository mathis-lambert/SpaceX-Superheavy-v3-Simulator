#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Misc/ConfigCacheIni.h"

void ARecoveryPlayerController::ApplyPhotoPreset(int32 Index)
{
    const auto& Presets=RecoveryPhotography::Presets();if(!Presets.IsValidIndex(Index))return;
    const auto& P=Presets[Index];Photography=P.Lens;TimeOfDay=P.Hour;FogAmount=P.Haze;
    MotionBlur=P.Blur;CameraGrain=P.Grain;bCameraDepthOfField=P.bDepthOfField;bAutomaticOrbit=P.bOrbit;
    StartingCamera=P.Camera;if(!IsAtHome())if(auto* D=GetDirector())D->Viewer->SetCameraMode(P.Camera);
    SavePreferences();
}
bool ARecoveryPlayerController::HasPhotoLook(int32 Slot) const
{
    bool Saved=false;
    if(Slot>=0 && Slot<3)GConfig->GetBool(*FString::Printf(TEXT("Recovery.PhotoLook.%d"),Slot),TEXT("Saved"),Saved,GGameUserSettingsIni);
    return Saved;
}
void ARecoveryPlayerController::SavePhotoLook(int32 Slot)
{
    if(Slot<0 || Slot>=3)return;
    const FString Section=FString::Printf(TEXT("Recovery.PhotoLook.%d"),Slot);
    Photography.Sanitize();Photography.Save(Section,GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("Hour"),TimeOfDay,GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("Haze"),FogAmount,GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("Blur"),MotionBlur,GGameUserSettingsIni);
    GConfig->SetFloat(*Section,TEXT("Grain"),CameraGrain,GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("DepthOfField"),bCameraDepthOfField,GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("Orbit"),bAutomaticOrbit,GGameUserSettingsIni);
    const auto* D=GetDirector();GConfig->SetInt(*Section,TEXT("Camera"),!IsAtHome() && D?D->Viewer->GetCameraMode():StartingCamera,GGameUserSettingsIni);
    GConfig->SetBool(*Section,TEXT("Saved"),true,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
}
void ARecoveryPlayerController::LoadPhotoLook(int32 Slot)
{
    if(!HasPhotoLook(Slot))return;
    const FString Section=FString::Printf(TEXT("Recovery.PhotoLook.%d"),Slot);
    Photography.Load(Section,GGameUserSettingsIni);
    GConfig->GetFloat(*Section,TEXT("Hour"),TimeOfDay,GGameUserSettingsIni);
    GConfig->GetFloat(*Section,TEXT("Haze"),FogAmount,GGameUserSettingsIni);
    GConfig->GetFloat(*Section,TEXT("Blur"),MotionBlur,GGameUserSettingsIni);
    GConfig->GetFloat(*Section,TEXT("Grain"),CameraGrain,GGameUserSettingsIni);
    GConfig->GetBool(*Section,TEXT("DepthOfField"),bCameraDepthOfField,GGameUserSettingsIni);
    GConfig->GetBool(*Section,TEXT("Orbit"),bAutomaticOrbit,GGameUserSettingsIni);
    GConfig->GetInt(*Section,TEXT("Camera"),StartingCamera,GGameUserSettingsIni);
    const auto Safe=[](float V,float Min,float Max,float Default){return FMath::IsFinite(V)?FMath::Clamp(V,Min,Max):Default;};
    TimeOfDay=Safe(TimeOfDay,0,24,12);FogAmount=Safe(FogAmount,0,2,1);
    MotionBlur=Safe(MotionBlur,0,.5f,.15f);CameraGrain=Safe(CameraGrain,0,.35f,.025f);
    StartingCamera=FMath::Clamp(StartingCamera,0,URecoveryCameraComponent::CameraCount-1);
    if(!IsAtHome())if(auto* D=GetDirector())D->Viewer->SetCameraMode(StartingCamera);
    SavePreferences();
}

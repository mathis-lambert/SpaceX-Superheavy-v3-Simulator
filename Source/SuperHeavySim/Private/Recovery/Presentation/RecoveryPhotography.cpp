#include "Recovery/Presentation/RecoveryPhotography.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
    struct FPhotoConfigField { const TCHAR* Name;float FRecoveryPhotography::*Value;float Min,Max; };
    const FPhotoConfigField Fields[]={
        {TEXT("FocalLengthMm"),&FRecoveryPhotography::FocalLengthMm,12,600},
        {TEXT("ExposureBiasEV"),&FRecoveryPhotography::ExposureBiasEV,-3,3},
        {TEXT("WhiteBalanceK"),&FRecoveryPhotography::WhiteBalanceK,2500,10000},
        {TEXT("Tint"),&FRecoveryPhotography::Tint,-.3f,.3f},
        {TEXT("Saturation"),&FRecoveryPhotography::Saturation,0,1.5f},
        {TEXT("Contrast"),&FRecoveryPhotography::Contrast,.7f,1.3f},
        {TEXT("Aperture"),&FRecoveryPhotography::Aperture,1.4f,22},
        {TEXT("FocusDistanceM"),&FRecoveryPhotography::FocusDistanceM,2,20000},
        {TEXT("MotionStrength"),&FRecoveryPhotography::MotionStrength,0,1},
        {TEXT("OrbitSpeed"),&FRecoveryPhotography::OrbitSpeed,0,3},
        {TEXT("TrackingLagSeconds"),&FRecoveryPhotography::TrackingLagSeconds,0,.5f},
        {TEXT("SolarDayOfYear"),&FRecoveryPhotography::SolarDayOfYear,1,365},
        {TEXT("UtcOffsetHours"),&FRecoveryPhotography::UtcOffsetHours,-6,-5}};
}
void FRecoveryPhotography::Sanitize()
{
    const FRecoveryPhotography Defaults;
    for(const auto& F:Fields){auto& V=this->*F.Value;V=FMath::IsFinite(V)?FMath::Clamp(V,F.Min,F.Max):Defaults.*F.Value;}
}
void FRecoveryPhotography::Load(const FString& Section,const FString& Ini)
{
    for(const auto& F:Fields)GConfig->GetFloat(*Section,F.Name,this->*F.Value,Ini);
    GConfig->GetBool(*Section,TEXT("AutomaticFraming"),bAutomaticFraming,Ini);
    GConfig->GetBool(*Section,TEXT("AutomaticFocus"),bAutomaticFocus,Ini);
    GConfig->GetBool(*Section,TEXT("FixedFraming"),bFixedFraming,Ini);Sanitize();
}
void FRecoveryPhotography::Save(const FString& Section,const FString& Ini) const
{
    for(const auto& F:Fields)GConfig->SetFloat(*Section,F.Name,this->*F.Value,Ini);
    GConfig->SetBool(*Section,TEXT("AutomaticFraming"),bAutomaticFraming,Ini);
    GConfig->SetBool(*Section,TEXT("AutomaticFocus"),bAutomaticFocus,Ini);
    GConfig->SetBool(*Section,TEXT("FixedFraming"),bFixedFraming,Ini);
}
double FRecoveryPhotography::HorizontalFovDegrees() const
{return FMath::RadiansToDegrees(2*FMath::Atan(18./FocalLengthMm));}

const TArray<FRecoveryPhotoPreset>& RecoveryPhotography::Presets()
{
    static const TArray<FRecoveryPhotoPreset> Values=[]()
    {
        TArray<FRecoveryPhotoPreset> P;
        FRecoveryPhotoPreset Neutral{TEXT("Natural daylight")};Neutral.Camera=0;P.Add(Neutral);
        auto Broadcast=Neutral;Broadcast.Name=TEXT("Broadcast telephoto");Broadcast.Camera=1;
        Broadcast.Lens.FocalLengthMm=135;Broadcast.Lens.bAutomaticFraming=false;Broadcast.Lens.Aperture=11;
        Broadcast.Lens.WhiteBalanceK=5900;Broadcast.Lens.Saturation=.92f;Broadcast.Lens.Contrast=1.04f;
        Broadcast.Lens.MotionStrength=.12f;Broadcast.Lens.TrackingLagSeconds=.09f;
        Broadcast.Haze=.7f;Broadcast.Grain=.035f;P.Add(Broadcast);
        auto Onboard=Neutral;Onboard.Name=TEXT("Onboard camera");Onboard.Camera=5;
        Onboard.Lens.FocalLengthMm=20;Onboard.Lens.bAutomaticFraming=false;Onboard.Lens.Aperture=11;
        Onboard.Lens.Saturation=.9f;Onboard.Lens.MotionStrength=.25f;Onboard.Blur=.12f;Onboard.Grain=.04f;P.Add(Onboard);
        auto Sunset=Neutral;Sunset.Name=TEXT("Golden hour");Sunset.Camera=7;Sunset.Hour=19.05f;
        Sunset.Lens.WhiteBalanceK=6200;Sunset.Lens.ExposureBiasEV=.15f;Sunset.Lens.Aperture=5.6f;
        Sunset.Lens.Saturation=1.06f;Sunset.Haze=.8f;Sunset.bOrbit=true;Sunset.Blur=.2f;P.Add(Sunset);
        auto Night=Neutral;Night.Name=TEXT("Industrial night");Night.Camera=11;Night.Hour=21.f;
        Night.Lens.WhiteBalanceK=5000;Night.Lens.ExposureBiasEV=1.5f;Night.Lens.Contrast=1.03f;
        Night.Lens.Aperture=4.f;Night.Haze=.55f;Night.Grain=.055f;Night.Blur=.18f;P.Add(Night);
        auto Fixed=Neutral;Fixed.Name=TEXT("Fixed coastal camera");Fixed.Camera=9;
        Fixed.Lens.bFixedFraming=true;Fixed.Lens.bAutomaticFraming=false;
        Fixed.Lens.FocalLengthMm=85;Fixed.Lens.Aperture=11;Fixed.Grain=.015f;P.Add(Fixed);
        return P;
    }();
    return Values;
}

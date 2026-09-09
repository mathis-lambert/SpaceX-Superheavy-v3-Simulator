#pragma once
#include "CoreMinimal.h"

/** Viewer optics and grading only. Values never enter the flight model. */
struct FRecoveryPhotography
{
    float FocalLengthMm=35.f;
    float ExposureBiasEV=0.f;
    float WhiteBalanceK=6500.f;
    float Tint=0.f;
    float Saturation=1.f;
    float Contrast=1.f;
    float Aperture=8.f;
    float FocusDistanceM=500.f;
    float MotionStrength=0.f;
    float OrbitSpeed=1.f;
    float SolarDayOfYear=252.f;
    float UtcOffsetHours=-5.f;
    bool bAutomaticFraming=true;
    bool bAutomaticFocus=true;
    void Sanitize();
    void Load(const FString& Section,const FString& Ini);
    void Save(const FString& Section,const FString& Ini) const;
    double HorizontalFovDegrees() const;
};

struct FRecoveryPhotoPreset
{
    const TCHAR* Name;
    FRecoveryPhotography Lens;
    float Hour=12.f,Haze=1.f,Blur=.15f,Grain=.025f;
    bool bDepthOfField=true,bOrbit=false;
    int32 Camera=0;
};

namespace RecoveryPhotography
{
    const TArray<FRecoveryPhotoPreset>& Presets();
}

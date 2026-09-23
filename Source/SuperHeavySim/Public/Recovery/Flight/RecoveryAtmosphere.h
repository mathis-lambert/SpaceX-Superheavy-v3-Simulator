#pragma once
#include "CoreMinimal.h"
namespace RecoveryAtmosphere
{
    constexpr double G0=9.80665;
    constexpr double EarthRadiusM=6356766.;
    struct FState { double Density,Pressure,Temperature,SoundSpeed,Gravity; };
    inline FVector WindAt(const FVector& SurfaceWindMps,double Height,double Scale)
    {
        return SurfaceWindMps*Scale*(.4+.6*FMath::Clamp(Height/100.,0.,1.))*
            FMath::Exp(-FMath::Max(0.,Height-10000.)/18000.);
    }
    // US Standard Atmosphere 1976 to 86 km geometric; exponential continuation above.
    inline FState Sample(double AltitudeM,double TemperatureOffset=0)
    {
        const double H=FMath::Max(0.,AltitudeM),Z=EarthRadiusM*H/(EarthRadiusM+H);
        constexpr double Heights[]={0,11000,20000,32000,47000,51000,71000,84852};
        constexpr double Lapse[]={-0.0065,0,0.001,0.0028,0,-0.0028,-0.002,0};
        constexpr double Temps[]={288.15,216.65,216.65,228.65,270.65,270.65,214.65,186.946};
        constexpr double Pressures[]={101325,22632.06,5474.889,868.019,110.906,66.9389,3.95642,0.373384};
        int I=0; while(I<7 && Z>=Heights[I+1]) ++I;
        const double T=Temps[I]+Lapse[I]*(Z-Heights[I]);
        const double P=Pressures[I]*(Lapse[I]==0 ? FMath::Exp(-G0*(Z-Heights[I])/(287.05287*T)) : FMath::Pow(Temps[I]/T,G0/(287.05287*Lapse[I])));
        const double ActualT=T+TemperatureOffset*FMath::Exp(-H/11000.);
        return {P/(287.05287*ActualT),P,ActualT,FMath::Sqrt(1.4*287.05287*ActualT),G0*FMath::Square(EarthRadiusM/(EarthRadiusM+H))};
    }
}

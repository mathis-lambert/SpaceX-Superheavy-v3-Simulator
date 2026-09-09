#pragma once
#include "CoreMinimal.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"

// Read-only sound design, SI units. This is a bounded straight-ray acoustic
// approximation, not a solver for shock waves, refraction or calibrated SPL.
namespace RecoveryAcoustics
{
    inline double Air(double AltitudeM)
    { return 1-FMath::SmoothStep(8000.,45000.,AltitudeM); }
    inline double TravelSeconds(const FVector& SourceM,const FVector& ListenerM,double TemperatureOffsetK=8)
    {
        const double C=RecoveryAtmosphere::Sample(FMath::Max(0.,(SourceM.Z+ListenerM.Z)*.5),TemperatureOffsetK).SoundSpeed;
        return FVector::Distance(SourceM,ListenerM)/FMath::Max(250.,C);
    }
    inline double Gain(double DistanceM,double Power)
    { return FMath::Sqrt(FMath::Clamp(Power,0.,1.))/(1+FMath::Max(0.,DistanceM)/450.); }
    inline double Smooth(double Current,double Target,double Dt,double Attack=.065,double Release=.22)
    { return FMath::Lerp(Current,Target,1-FMath::Exp(-FMath::Max(0.,Dt)/FMath::Max(.001,Target>Current?Attack:Release))); }
    struct FSample
    {
        double Time=0;
        FVector PositionM=FVector::ZeroVector,VelocityMps=FVector::ZeroVector;
        FVector4 Channels=FVector4(0,0,0,0);
    };
    class FHistory
    {
        static constexpr int Capacity=8192;
        TArray<FSample> Samples;
        int Next=0;
        const FSample& Newest(int Offset) const { return Samples[(Next-1-Offset+Samples.Num())%Samples.Num()]; }
    public:
        void Reset() { Samples.Reset();Next=0; }
        int Num() const { return Samples.Num(); }
        void Add(const FSample& Sample)
        {
            if(Samples.Num() && Sample.Time<Newest(0).Time)Reset();
            if(Samples.Num()<Capacity){Samples.Add(Sample);Next=Samples.Num()%Capacity;}
            else {Samples[Next]=Sample;Next=(Next+1)%Capacity;}
        }
        FSample Hear(double Time,const FVector& ListenerM,double TemperatureOffsetK=8) const
        {
            FSample Silent;Silent.Time=Time;Silent.PositionM=ListenerM;
            for(int I=0;I<Samples.Num();++I)
            {
                const auto& A=Newest(I);
                const double Arrival=A.Time+TravelSeconds(A.PositionM,ListenerM,TemperatureOffsetK);
                if(Arrival>Time)continue;
                // Select the latest arriving source branch. Supersonic multiple
                // arrivals are deliberately not synthesized as sonic booms.
                if(I==0)return A;
                const auto& B=Newest(I-1);
                const double End=B.Time+TravelSeconds(B.PositionM,ListenerM,TemperatureOffsetK);
                if(End<=Arrival)return A;
                const double Alpha=FMath::Clamp((Time-Arrival)/(End-Arrival),0.,1.);
                return {FMath::Lerp(A.Time,B.Time,Alpha),FMath::Lerp(A.PositionM,B.PositionM,Alpha),
                    FMath::Lerp(A.VelocityMps,B.VelocityMps,Alpha),FMath::Lerp(A.Channels,B.Channels,Alpha)};
            }
            return Silent;
        }
    };
    inline double Doppler(const FVector& SourceVelocity,const FVector& ListenerVelocity,const FVector& Ray,double SoundSpeed=343)
    {
        const double Denominator=SoundSpeed-FVector::DotProduct(SourceVelocity,Ray);
        // Pitch is bounded for intelligibility near the Mach cone and on camera cuts.
        return FMath::Clamp((SoundSpeed-FVector::DotProduct(ListenerVelocity,Ray))/FMath::Max(SoundSpeed*.5,Denominator),.65,1.45);
    }
    inline double VibrationDegrees(double HeardPower,double DistanceM,double StructuralPower,double DynamicPressurePa,bool Onboard)
    {
        const double Acoustic=.08*Gain(DistanceM,HeardPower);
        const double Structure=Onboard?(.035*FMath::Sqrt(FMath::Clamp(StructuralPower,0.,1.))+.025*FMath::Clamp(DynamicPressurePa/40000.,0.,1.)):0;
        return FMath::Clamp(Acoustic+Structure,0.,.12);
    }
}

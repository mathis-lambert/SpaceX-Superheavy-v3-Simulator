#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

namespace FlightGeometry
{
    inline constexpr double CentimetersPerMeter = 100.;
    inline constexpr double BoosterBaseOffsetM = 35.44;
    inline constexpr double UpperStageBaseHeightM = 71.02;
    inline constexpr double UpperStageCentreFromBaseM = 25.;
    inline constexpr double EarthRadiusM = 6371000.;

    inline FVector BoosterBaseCm(const UPrimitiveComponent& Body)
    {
        return Body.GetComponentLocation() - Body.GetUpVector() * BoosterBaseOffsetM * CentimetersPerMeter;
    }

    inline FVector EarthCenterCm() { return FVector(0, 0, -EarthRadiusM * CentimetersPerMeter); }

    inline double AltitudeM(const FVector& PositionCm)
    {
        return (PositionCm - EarthCenterCm()).Size() / CentimetersPerMeter - EarthRadiusM;
    }

    inline const TArray<FVector>& ReactionNozzlePositionsM()
    {
        static const TArray<FVector> Positions = {
            {4.55, 0, 62}, {4.55, 0, 12}, {-4.55, 0, 62},
            {-4.55, 0, 12}, {0, 4.55, 52}, {0, -4.55, 52}};
        return Positions;
    }
    inline const TArray<FVector>& UpperStageNozzlePositionsM()
    {
        static const TArray<FVector> Positions=[]
        {
            TArray<FVector> Result;
            for(int I=0;I<6;++I)
            {
                const double Angle=(I%3)*2*PI/3+(I>=3?PI/3:0);
                const double Radius=I<3?1.5:3.3;
                Result.Add(FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0));
            }
            return Result;
        }();
        return Positions;
    }
    inline const TArray<FVector>& ConditioningVentPositionsM()
    {
        static const TArray<FVector> Positions={{4.45,0,51},{0,-4.45,13}};
        return Positions;
    }
    inline const TArray<FVector>& ConditioningVentDirections()
    {
        static const TArray<FVector> Directions={{1,0,0},{0,-1,0}};
        return Directions;
    }
}

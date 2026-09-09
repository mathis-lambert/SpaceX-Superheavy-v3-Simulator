#pragma once
#include "CoreMinimal.h"

/** NOAA fractional-year approximation. X east, Y true north, Z up.
 * Civil UTC offset includes daylight saving (Starbase CDT = -5, CST = -6).
 * Atmospheric refraction is intentionally excluded from geometric direction.
 */
namespace RecoverySolarPosition
{
    inline FVector Direction(double LatitudeDeg,double LongitudeDeg,double LocalHour,double UtcOffsetHours,int32 DayOfYear)
    {
        const double Gamma=2*PI/365.*(FMath::Clamp(DayOfYear,1,365)-1+(LocalHour-UtcOffsetHours-12)/24.);
        const double Equation=229.18*(.000075+.001868*FMath::Cos(Gamma)-.032077*FMath::Sin(Gamma)-.014615*FMath::Cos(2*Gamma)-.040849*FMath::Sin(2*Gamma));
        const double Decl=.006918-.399912*FMath::Cos(Gamma)+.070257*FMath::Sin(Gamma)-.006758*FMath::Cos(2*Gamma)+.000907*FMath::Sin(2*Gamma)-.002697*FMath::Cos(3*Gamma)+.00148*FMath::Sin(3*Gamma);
        const double H=FMath::DegreesToRadians((LocalHour*60+Equation+4*LongitudeDeg-60*UtcOffsetHours)/4-180);
        const double Phi=FMath::DegreesToRadians(LatitudeDeg);
        return FVector(-FMath::Cos(Decl)*FMath::Sin(H),FMath::Cos(Phi)*FMath::Sin(Decl)-FMath::Sin(Phi)*FMath::Cos(Decl)*FMath::Cos(H),FMath::Sin(Phi)*FMath::Sin(Decl)+FMath::Cos(Phi)*FMath::Cos(Decl)*FMath::Cos(H));
    }
}

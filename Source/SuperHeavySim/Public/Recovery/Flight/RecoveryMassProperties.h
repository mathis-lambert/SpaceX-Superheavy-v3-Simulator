#pragma once

#include "CoreMinimal.h"
#include "Recovery/Flight/SuperHeavyRecoveryProfile.h"

class UPrimitiveComponent;

namespace RecoveryMass
{
    struct FProperties
    {
        double MassKg=0, CentreFromBaseM=0;
        FVector InertiaKgM2=FVector::ZeroVector;
    };
    struct FPart { double Mass,Z,Length,Radius;bool Shell=false; };

    inline FProperties Combine(const TArray<FPart,TInlineAllocator<6>>& Parts)
    {
        FProperties Out;
        for(const auto& Part:Parts) {Out.MassKg+=Part.Mass;Out.CentreFromBaseM+=Part.Mass*Part.Z;}
        Out.CentreFromBaseM/=FMath::Max(1.,Out.MassKg);
        for(const auto& Part:Parts)
        {
            const double R2=Part.Radius*Part.Radius;
            const double Axial=Part.Mass*R2*(Part.Shell?1.:.5);
            const double Transverse=Part.Mass*((Part.Shell?.5:.25)*R2+Part.Length*Part.Length/12.+FMath::Square(Part.Z-Out.CentreFromBaseM));
            Out.InertiaKgM2+=FVector(Transverse,Transverse,Axial);
        }
        return Out;
    }

    inline FProperties Booster(const USuperHeavyRecoveryProfile& P,double FuelKg,double GasKg,bool WithUpperStage)
    {
        // Estimated coaxial tank geometry and bulk densities. No mesh bounding
        // box is used as a substitute for a full stack's mass distribution.
        const double Oxygen=FuelKg*P.MixtureRatio/(1+P.MixtureRatio),Methane=FuelKg-Oxygen;
        const double Area=PI*4.5*4.5;
        const double OxygenDepth=Oxygen/(P.OxygenDensityKgM3*Area),MethaneDepth=Methane/(P.MethaneDensityKgM3*Area);
        TArray<FPart,TInlineAllocator<6>> Parts;
        Parts.Add({P.DryMassKg,35.44,70.88,4.5,true});
        Parts.Add({Oxygen,P.OxygenTankBottomM+OxygenDepth*.5,OxygenDepth,4.5,false});
        Parts.Add({Methane,P.MethaneTankBottomM+MethaneDepth*.5,MethaneDepth,4.5,false});
        Parts.Add({GasKg,42,50,4.55,true});
        if(WithUpperStage)Parts.Add({P.UpperStageMassKg,96.02,50,4.5,false});
        return Combine(Parts);
    }
    inline FProperties UpperStage(double MassKg)
    {
        TArray<FPart,TInlineAllocator<6>> Parts;
        Parts.Add({MassKg,25,50,4.5,false});
        return Combine(Parts);
    }
    void Apply(UPrimitiveComponent& Body,const FProperties& Properties,double BodyOriginFromBaseM);
}

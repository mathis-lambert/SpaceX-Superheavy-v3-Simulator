#include "Recovery/Flight/RecoveryWaterContact.h"
#include "Recovery/Shared/FlightGeometry.h"

double RecoveryWaterContact::Apply(const FRecoveryBodyKinematics& Body,const RecoveryMass::FProperties& Mass,
    double OriginFromBaseM,double HullBottomM,double HullLengthM,double RadiusM,
    const FRecoveryWaterMap* Map,double Dt,TArray<FRecoveryForceSample>& Forces)
{
    if(!Map || Dt<=0 || FlightGeometry::AltitudeM(Body.OriginM*100)>HullLengthM+RadiusM)return 0;
    const FVector Base=Body.OriginM-Body.Rotation.GetUpVector()*OriginFromBaseM;
    const FVector COM=Base+Body.Rotation.GetUpVector()*Mass.CentreFromBaseM;
    constexpr int32 Axial=16,Radial=5,Count=Axial*Radial;
    const double CellRadius=RadiusM*.52,CellVolume=PI*RadiusM*RadiusM*HullLengthM/Count;
    double Submerged=0;
    for(int32 I=0;I<Axial;++I)for(int32 J=0;J<Radial;++J)
    {
        const double Angle=J*PI*.5;
        const double R=J==4?0:RadiusM*.52;
        const FVector Point=Base+Body.Rotation.RotateVector(FVector(R*FMath::Cos(Angle),R*FMath::Sin(Angle),HullBottomM+(I+.5)*HullLengthM/Axial));
        const double Depth=CellRadius-FlightGeometry::AltitudeM(Point*100);
        if(Depth<=0 || !Map->IsWater(Point))continue;
        const double Cap=FMath::Clamp(Depth/CellRadius,0.,2.);
        const double Fraction=Cap*Cap*(3-Cap)*.25;
        const double Volume=CellVolume*Fraction;Submerged+=Volume;
        const FVector Up=(Point-FVector(0,0,-FlightGeometry::EarthRadiusM)).GetSafeNormal();
        const FVector Velocity=Body.VelocityMps+FVector::CrossProduct(Body.AngularVelocityWorldRadS,Point-COM);
        // Implicit quadratic drag is dissipative even at high entry speeds.
        // The cell mass budget prevents a single drag step reversing velocity.
        const double K=1025.*.5*(PI*CellRadius*CellRadius)*Fraction*(1.2*Velocity.Size()+4.);
        const double EffectiveK=K/(1+K*Dt/FMath::Max(1.,Mass.MassKg/(Count*4.)));
        Forces.Add({ERecoveryForceKind::Water,I*Radial+J,Point*100,Up*(1025.*9.80665*Volume)-Velocity*EffectiveK});
    }
    return Submerged;
}

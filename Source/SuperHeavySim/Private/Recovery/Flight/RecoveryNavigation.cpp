#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

FRecoveryNavigationState RecoveryNavigation::Evaluate(const FRecoveryGuidanceConfiguration& Config,const FRecoveryBodyKinematics& Body,
    double FuelKg,double RcsFuelKg,bool Separated,const FRecoveryFlightExperiment& Experiment)
{
    FRecoveryNavigationState N;
    N.BasePositionM=Body.OriginM-Body.Rotation.GetUpVector()*FlightGeometry::BoosterBaseOffsetM;
    N.VelocityMps=Body.VelocityMps;N.AltitudeM=FlightGeometry::AltitudeM(N.BasePositionM*100.);
    N.MassKg=RecoveryMass::Booster(Config,FuelKg,RcsFuelKg,!Separated).MassKg;
    const FVector Up=(N.BasePositionM*100.-FlightGeometry::EarthCenterCm()).GetSafeNormal();
    N.VerticalSpeedMps=FVector::DotProduct(N.VelocityMps,Up);
    N.HorizontalErrorM=FVector2D(N.BasePositionM-Config.CaptureWorldM).Size();
    N.TiltDeg=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Body.Rotation.GetUpVector(),Up),-1.,1.)));
    const auto Air=RecoveryAtmosphere::Sample(N.AltitudeM,Config.SeaLevelTemperatureOffsetK);
    N.DensityKgM3=Air.Density;N.PressurePa=Air.Pressure;N.Gravity=Air.Gravity;
    const FVector Relative=N.VelocityMps-RecoveryAtmosphere::WindAt(Config.WindVelocityMps,N.AltitudeM,Experiment.WindScale);
    N.DynamicPressurePa=.5*Air.Density*Relative.SizeSquared();N.Mach=Relative.Size()/Air.SoundSpeed;
    N.EngineIspS=FMath::Lerp(Config.SpecificImpulseVacuumS,Config.SpecificImpulseSeaLevelS,FMath::Clamp(Air.Pressure/101325.,0.,1.));
    const FVector Heading=Config.TowerRotation.UnrotateVector(Body.Rotation.GetForwardVector());
    N.HeadingErrorDeg=FMath::Abs(FMath::FindDeltaAngleDegrees(FMath::RadiansToDegrees(FMath::Atan2(Heading.Y,Heading.X)),Config.CaptureHeadingDeg));
    const FQuat CaptureQ=Config.TowerRotation*FQuat(FVector::UpVector,FMath::DegreesToRadians(Config.CaptureHeadingDeg));
    const FVector L1=N.BasePositionM+Body.Rotation.RotateVector(Config.CatchLugPlusM);
    const FVector L2=N.BasePositionM+Body.Rotation.RotateVector(Config.CatchLugMinusM);
    N.CatchLugErrorM=FMath::Max((L1-(Config.CaptureWorldM+CaptureQ.RotateVector(Config.CatchLugPlusM))).Size(),
        (L2-(Config.CaptureWorldM+CaptureQ.RotateVector(Config.CatchLugMinusM))).Size());
    const double Decel=FMath::Min(Config.LandingDecelerationMps2,13*Config.EngineThrustN/FMath::Max(1.,N.MassKg)-Air.Gravity);
    N.BrakingDistanceM=FMath::Square(FMath::Min(N.VelocityMps.Z,0.))/(2*FMath::Max(.1,Decel));
    return N;
}

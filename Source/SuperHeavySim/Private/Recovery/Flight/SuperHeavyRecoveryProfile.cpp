#include "Recovery/Flight/SuperHeavyRecoveryProfile.h"
#include "UObject/UnrealType.h"
bool USuperHeavyRecoveryProfile::Validate(FString& Reason) const
{
    for(TFieldIterator<FDoubleProperty> It(GetClass());It;++It)
        if(!FMath::IsFinite(It->GetPropertyValue_InContainer(this)))
        { Reason=TEXT("Every numeric mission parameter must be finite"); return false; }
    if(LaunchOffsetM.ContainsNaN() || WindVelocityMps.ContainsNaN() || CatchLugPlusM.ContainsNaN() || CatchLugMinusM.ContainsNaN() ||
       DryMassKg<100000 || PropellantMassKg<1000000 || UpperStageMassKg<0 || EngineThrustN<=0 ||
       33*EngineThrustN/(LaunchMassKg()*9.80665)<1.1 || 3*EngineThrustN/((DryMassKg+LandingReserveKg)*9.80665)<1.3 ||
       SpecificImpulseSeaLevelS<200 || SpecificImpulseVacuumS<SpecificImpulseSeaLevelS || MinimumThrottle<0.1 || MinimumThrottle>0.5 ||
       ApogeeM<70000 || ApogeeM>140000 || AscentDurationS<100 || AscentDurationS>180 || AscentPitchDeg<20 || AscentPitchDeg>75 ||
       TimeoutSeconds<300 || ThrottleTimeConstant<0.02 || MaxGimbalDeg<=0 || MaxGimbalDeg>15 || MaxTiltDeg<=0 || MaxTiltDeg>30 ||
       ConditioningVentKgS<0 || ConditioningJetSpeedMps<0 || MixtureRatio<=0 || OxygenDensityKgM3<=0 || MethaneDensityKgM3<=0 || OxygenTankBottomM<0 || MethaneTankBottomM<=OxygenTankBottomM ||
       GimbalRateDegS<=0 || EngineShutdownTimeS<=0 || ReactionValveTimeConstantS<=0 || UpperStageDryMassKg<=0 ||
       (UpperStageMassKg>0 && UpperStageDryMassKg>=UpperStageMassKg) || UpperStageIspS<=0 || UpperStageEngineThrustN<=0 ||
       GridFinAreaM2<=0 || GridFinLiftSlope<=0 || GridFinMaxAngleDeg<=0 || GridFinMaxAngleDeg>45 || GridFinRateDegS<=0 ||
       MaxEntryAngleDeg<=0 || MaxEntryAngleDeg>15 || AxialDragCoefficient<=0 || DragAreaM2<=0 || BodySideAreaM2<=0 || BodyNormalCoefficient<=0 ||
       ReactionControlTorqueNm<=0 || ReactionControlPropellantKg<=0 || LandingReserveKg<=0 || LandingReserveKg>PropellantMassKg*0.2 ||
       TailFirstDragCoefficient<=0 || GridFinDragCoefficient<0 || BoostbackReserveKg<=0 || BoostbackReserveKg+LandingReserveKg>PropellantMassKg ||
       FMath::Abs(LandingDriftCorrectionS)>60 || FrontReturnOffsetM<400 || FrontReturnOffsetM>5000 || LandingWindLeadS<0 || LandingWindLeadS>60 || LandingIgnitionCeilingM<500 || LandingIgnitionCeilingM>10000 ||
       AscentMaxAccelerationMps2<10 || AscentMaxAccelerationMps2>50 || FMath::Abs(SeaLevelTemperatureOffsetK)>40 ||
       LandingDecelerationMps2<=0 || LandingBurnMarginM<0 || CaptureRadiusM<=0 || CaptureSpeedMps<=0 || CaptureDwellSeconds<=0 ||
       CaptureTiltDeg<=0 || CaptureTiltDeg>5 || CaptureHeadingToleranceDeg<=0 || CaptureHeadingToleranceDeg>10)
    { Reason=TEXT("Invalid mass, thrust, trajectory, actuator or capture configuration"); return false; }
    Reason.Empty(); return true;
}

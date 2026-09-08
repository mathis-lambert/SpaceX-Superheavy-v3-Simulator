#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Tests/RecoveryDiagnosticsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Presentation/RecoveryPresentationComponent.h"
#include "Recovery/Presentation/RecoverySkyComponent.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Vehicle/SuperHeavyVehicleActor.h"
#include "Autopilot/SuperHeavyAutopilotComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/ChildActorComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogRecovery, Log, All);

FVector ASuperHeavyRecoveryDirector::AttitudeTorque(const FVector& TargetUp,double Gain,double Damping) const
{
    const FQuat Q=Body->GetComponentQuat();
    const FVector Heading=Tower->GetActorQuat().RotateVector(FRotator(0,RuntimeProfile->CaptureHeadingDeg,0).Vector());
    const FQuat TargetQ=FRotationMatrix::MakeFromZX(TargetUp,Heading).ToQuat();
    FQuat Err=TargetQ*Q.Inverse(); Err.Normalize(); if(Err.W<0) Err=Err*-1.;
    FVector Axis; double Angle; Err.ToAxisAndAngle(Axis,Angle);
    const FVector Inertia=Body->GetInertiaTensor()/10000.;
    const double EffectiveInertia=FMath::Max(1.,(Inertia*Q.UnrotateVector(Axis)).Size());
    const double Authority=ActualThrustN*FMath::Tan(FMath::DegreesToRadians(RuntimeProfile->MaxGimbalDeg))*31+
        (RcsPropellantKg>0 ? RuntimeProfile->ReactionControlTorqueNm : 0)+
        DynamicPressurePa*RuntimeProfile->GridFinAreaM2*RuntimeProfile->GridFinLiftSlope*0.4*28.8;
    const double BrakingRate=0.65*FMath::Sqrt(2*FMath::Max(0.00001,Authority/EffectiveInertia)*Angle);
    const double Rate=FMath::Min3(0.20,BrakingRate,Angle*Gain/Damping);
    const FVector Alpha=(Axis*Rate-Body->GetPhysicsAngularVelocityInRadians())*(Damping*Experiment.AttitudeResponse);
    const FVector Omega=Q.UnrotateVector(Body->GetPhysicsAngularVelocityInRadians());
    return Inertia*Q.UnrotateVector(Alpha)+FVector::CrossProduct(Omega,Inertia*Omega);
}
void ASuperHeavyRecoveryDirector::Guide(double Dt)
{
    if(!ContactFixture.IsEmpty()) { TickContactFixture(Dt);return; }
    PredictorClock+=Dt;
    if(PredictorClock>=0.25) { PredictorClock=0; PredictBallistic(); }
    const FVector Downrange=Tower->GetActorForwardVector();
    const double AvailablePerEngine=RuntimeProfile->EngineThrustN*EngineIspS/RuntimeProfile->SpecificImpulseSeaLevelS;
    FVector ForceAccel=FVector::ZeroVector,TargetUp=FVector::UpVector;
    ActiveEngines=0;
    TargetPositionM=CaptureWorldM;
    if(Phase==ERecoveryPhase::Ascent)
    {
        const double Pitch=FMath::DegreesToRadians(RuntimeProfile->AscentPitchDeg)*FMath::SmoothStep(10.,RuntimeProfile->AscentDurationS,PhaseTime);
        TargetUp=FVector::UpVector*FMath::Cos(Pitch)+Downrange*FMath::Sin(Pitch);
        double Load=FMath::Min(33*AvailablePerEngine/MassKg,RuntimeProfile->AscentMaxAccelerationMps2);
        if(DynamicPressurePa>35000) Load*=FMath::Clamp(35000./DynamicPressurePa,0.65,1.);
        ActiveEngines=33; ForceAccel=TargetUp*Load;
        if(PhaseTime>=RuntimeProfile->AscentDurationS || PropellantKg<RuntimeProfile->LandingReserveKg+RuntimeProfile->BoostbackReserveKg)
        {
            SeparateUpperStage();
            SetPhase(ERecoveryPhase::Separation,TEXT("MECO / stage separation / return attitude"));
        }
    }
    if(Phase==ERecoveryPhase::Separation || Phase==ERecoveryPhase::Boostback)
    {
        const double Tgo=FMath::Max(40.,TimeToImpactS);
        FVector DV=(CaptureWorldM-PredictedImpactM)*(1.18/Tgo); DV.Z=0;
        const double ApogeeError=RuntimeProfile->ApogeeM-AltitudeM;
        const double DesiredVz=FMath::Sign(ApogeeError)*FMath::Sqrt(2*Gravity*FMath::Abs(ApogeeError))*0.75;
        ForceAccel=DV/6.; ForceAccel.Z=(DesiredVz-VerticalSpeedMps)/8.+Gravity;
        TargetUp=ForceAccel.GetSafeNormal();
        ActiveEngines=Phase==ERecoveryPhase::Boostback ? 13 : 0;
        if(Phase==ERecoveryPhase::Separation && PhaseTime>2 && FVector::DotProduct(TargetUp,Body->GetUpVector())>0.97)
        {
            BoostbackIgnitionAltitudeM=AltitudeM; BoostbackDownrangeM=HorizontalErrorM;
            SetPhase(ERecoveryPhase::Boostback,TEXT("13-engine boostback / solving ballistic return")); ActiveEngines=13;
        }
        if(Phase==ERecoveryPhase::Boostback)
        {
            BoostbackSeconds+=Dt;
            const double PredictedApogee=AltitudeM+FMath::Square(FMath::Max(0.,VerticalSpeedMps))/(2*Gravity);
            if(PhaseTime>5 && PredictedMissM<500 && PredictedApogee<RuntimeProfile->ApogeeM+3000)
            { SetPhase(ERecoveryPhase::Coast,TEXT("Boostback cutoff / unpowered coast to apogee")); ActiveEngines=0; }
            else if(PropellantKg<RuntimeProfile->LandingReserveKg)
            { SetPhase(ERecoveryPhase::Aborted,TEXT("Boostback depleted landing reserve")); WriteResult(false,StatusMessage); ActiveEngines=0; }
        }
    }
    if(Phase==ERecoveryPhase::Coast && VerticalSpeedMps<0 && DynamicPressurePa>200)
        SetPhase(ERecoveryPhase::Entry,TEXT("Engines OFF / atmospheric descent / grid-fin guidance"));
    if(Phase==ERecoveryPhase::Coast || Phase==ERecoveryPhase::Entry)
    {
        ActiveEngines=0; ForceAccel=FVector::ZeroVector;
        // Tail-first attitude and aerodynamic correction of the predicted entry point.
        const FVector Rel=VelocityMps-WindAt(AltitudeM);
        TargetUp=Rel.Z<-30 ? -Rel.GetSafeNormal() : FVector::UpVector;
        if(Phase==ERecoveryPhase::Entry)
        {
            const double LookAhead=FMath::Max(8.,TimeToImpactS);
            FVector DesiredA=(CaptureWorldM-PredictedImpactM)*(2./(LookAhead*LookAhead)); DesiredA.Z=0;
            const double Authority=DynamicPressurePa*RuntimeProfile->BodySideAreaM2*RuntimeProfile->BodyNormalCoefficient/FMath::Max(1.,MassKg);
            FVector Correction=(-DesiredA/FMath::Max(0.1,Authority)).GetClampedToMaxSize(FMath::Tan(FMath::DegreesToRadians(RuntimeProfile->MaxEntryAngleDeg)));
            TargetUp=(TargetUp+Correction).GetSafeNormal();
        }
        // Ignition follows energy and available deceleration, not a fixed timer.
        if(VerticalSpeedMps<-20 && AltitudeM<RuntimeProfile->LandingIgnitionCeilingM && AltitudeM-CaptureWorldM.Z<=BrakingDistanceM+RuntimeProfile->LandingBurnMarginM)
        {
            LandingIgnitionAltitudeM=AltitudeM;
            SetPhase(ERecoveryPhase::LandingBurn,TEXT("Landing burn / 13 to 3 Raptor engines")); IntegralXY=FVector::ZeroVector;
        }
    }
    if(Phase==ERecoveryPhase::LandingBurn || Phase==ERecoveryPhase::Capture)
    {
        LandingBurnSeconds+=Dt;
        if(Phase==ERecoveryPhase::LandingBurn && AltitudeM<500 && HorizontalErrorM<40 &&
            FVector2D(VelocityMps).Size()<4 && TiltDeg<5 && HeadingErrorDeg<3)
            SetPhase(ERecoveryPhase::Capture,TEXT("Final descent / catch-fittings and heading alignment"));
        // Keep the base above the tower until the terminal corridor is acquired.
        // This gate is geometric and re-evaluated from measured flight state.
        // A crosswind requires a small steady lean. Rejecting that necessary
        // attitude would leave the vehicle hovering until its reserve runs out.
        if(Phase==ERecoveryPhase::Capture && HorizontalErrorM<2.0 && FVector2D(VelocityMps).Size()<0.7 && TiltDeg<4. && HeadingErrorDeg<2.) bApproachAligned=true;
        const double Clearance=Phase==ERecoveryPhase::LandingBurn || !bApproachAligned ? Tower->TowerHeightM+30 : 0;
        // Translate the centre-of-mass target so the actual pair of fittings,
        // rather than the base axis, reaches the supports while leaning into wind.
        const FVector LugMid=(RuntimeProfile->CatchLugPlusM+RuntimeProfile->CatchLugMinusM)*0.5;
        const FQuat CaptureQ=Tower->GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(RuntimeProfile->CaptureHeadingDeg));
        const FVector PoseCorrection=CaptureQ.RotateVector(LugMid)-Body->GetComponentQuat().RotateVector(LugMid-FVector(0,0,BaseOffsetM))-FVector(0,0,BaseOffsetM);
        const double PoseBlend=Phase==ERecoveryPhase::Capture ? 1-FMath::SmoothStep(30.,130.,AltitudeM-CaptureWorldM.Z) : 0;
        // Acquire the opening from tower-local +X, then descend between rails.
        // The standoff vanishes while the entire booster is still above the arms.
        const double Standoff=Phase==ERecoveryPhase::Capture?0.:18.;
        const FVector Error=CaptureWorldM+Tower->GetActorForwardVector()*Standoff+FVector(0,0,BaseOffsetM+Clearance-0.25)+PoseCorrection*PoseBlend-Body->GetComponentLocation()/100;
        const double Height=FMath::Max(0.,Error.Z*-1);
        const double VerticalGain=FMath::Lerp(0.22,0.7,FMath::Clamp((Height-10)/90.,0.,1.));
        const double TerminalDecel=FMath::Min(RuntimeProfile->LandingDecelerationMps2,0.7*(3*AvailablePerEngine/MassKg-Gravity));
        const double DesiredVz=Error.Z>0 ? FMath::Min(3.,Error.Z*0.5) : -FMath::Min(FMath::Sqrt(2*FMath::Max(1.,TerminalDecel)*Height),FMath::Max(Phase==ERecoveryPhase::Capture?0.25:0.,Height*VerticalGain));
        FVector Lateral(Error.X,Error.Y,0);
        FVector DesiredVelocity=Lateral*0.12;
        DesiredVelocity=DesiredVelocity.GetClampedToMaxSize(FMath::Max(Phase==ERecoveryPhase::LandingBurn ? 40. : 12.,FMath::Abs(VerticalSpeedMps)*0.35));
        IntegralXY=FVector::ZeroVector;
        const double VelocityGain=AltitudeM<200 ? 0.35 : 0.3;
        ForceAccel=(DesiredVelocity-VelocityMps)*VelocityGain;
        ForceAccel.Z=Gravity+FMath::Clamp((DesiredVz-VerticalSpeedMps)*0.8,-4.,13*AvailablePerEngine/MassKg-Gravity);
        // Compensate measured modelled aerodynamic force as atmospheric speed falls.
        ForceAccel-=AeroForceN/FMath::Max(1.,MassKg);
        // Aerodynamic braking can exceed the requested deceleration. The engine
        // cannot thrust downward; retain an upward attitude through that handover.
        ForceAccel.Z=FMath::Max(Gravity*0.5,ForceAccel.Z);
        // In tail-first flight a tilted body's aerodynamic normal force can
        // exceed the lateral thrust. Solve that coupling instead of repeatedly
        // feeding the previous frame's side force back into the gimbal command.
        const FVector AirVelocity=VelocityMps-WindAt(AltitudeM);
        const double AirSpeed=FMath::Max(1.,AirVelocity.Size());
        const double NormalAuthority=DynamicPressurePa*RuntimeProfile->BodySideAreaM2*RuntimeProfile->BodyNormalCoefficient/MassKg;
        const double CdA=RuntimeProfile->DragAreaM2*RuntimeProfile->TailFirstDragCoefficient+3*RuntimeProfile->GridFinAreaM2*RuntimeProfile->GridFinDragCoefficient;
        const FVector AxialDrag=-DynamicPressurePa*CdA*AirVelocity/(AirSpeed*MassKg);
        const FVector DesiredA=(DesiredVelocity-VelocityMps)*VelocityGain;
        const double CoupledAuthority=ForceAccel.Z-NormalAuthority;
        FVector Tilt;
        if(FMath::Abs(CoupledAuthority)>ForceAccel.Z*0.2)
            Tilt=FVector(DesiredA.X-AxialDrag.X+NormalAuthority*AirVelocity.X/AirSpeed,DesiredA.Y-AxialDrag.Y+NormalAuthority*AirVelocity.Y/AirSpeed,0)/CoupledAuthority;
        else Tilt=-FVector(AirVelocity.X,AirVelocity.Y,0)/FMath::Max(20.,FMath::Abs(AirVelocity.Z));
        Tilt=Tilt.GetClampedToMaxSize(FMath::Tan(FMath::DegreesToRadians(RuntimeProfile->MaxTiltDeg)));
        // At low speed use the actual force model, including crosswind and fin
        // projected area. The tail-first approximation is only valid in descent.
        if(AirSpeed<50)
            Tilt=FVector(ForceAccel.X,ForceAccel.Y,0).GetClampedToMaxSize(ForceAccel.Z*FMath::Tan(FMath::DegreesToRadians(RuntimeProfile->MaxTiltDeg)))/ForceAccel.Z;
        TargetUp=FVector(Tilt.X,Tilt.Y,1).GetSafeNormal();
        ForceAccel=TargetUp*(ForceAccel.Z/TargetUp.Z);
        const double Required=ForceAccel.Size()*MassKg;
        // Separate engage/disengage thresholds prevent the guidance command
        // from restarting ten engines on consecutive frames near one boundary.
        // This changes engine requests only; physical valve dynamics still apply.
        const double ThreeEngineDemand=Required/(3*AvailablePerEngine);
        LandingEngineGroup=LandingEngineGroup>=13 ? (ThreeEngineDemand<.80?3:13) : (ThreeEngineDemand>.96?13:3);
        ActiveEngines=LandingEngineGroup;
        if(Phase==ERecoveryPhase::Capture)
        {
            Tower->SetArmClosure(FMath::Clamp((60.-Height)/25.,0.,1.));
            // Transfer weight on the first verified fitting contact. Continuing
            // hover thrust would hold the other fitting a few centimetres above
            // its rail in crosswind. Gravity settles both supports physically.
            if(SupportContactCount>0 && CatchLugErrorM<0.5 && VelocityMps.Size()<1.5 && TiltDeg<3 && !bContactShutdown)
            {
                bContactShutdown=true;
                UE_LOG(LogRecovery,Display,TEXT("CONTACT_SHUTDOWN t=%.3f speed=%.3f impulse=%.1f/%.1f"),MissionTime,VelocityMps.Size(),SupportImpulseNs.X,SupportImpulseNs.Y);
            }
            if(bContactShutdown) { ActiveEngines=0;ForceAccel=FVector::ZeroVector; }
            const bool Supported=bContactShutdown && SupportContactCount==2 && VelocityMps.Size()<0.5 && TiltDeg<4.;
            SettledContactSeconds=Supported?SettledContactSeconds+Dt:0;
            CaptureDwell=SettledContactSeconds;
            if(SettledContactSeconds>=0.6)
            {
                CaptureErrorAtLatch=HorizontalErrorM; CaptureSpeedAtLatch=VelocityMps.Size(); CaptureTiltAtLatch=TiltDeg;
                CaptureHeadingAtLatch=HeadingErrorDeg; CaptureLugAtLatch=CatchLugErrorM; LatchPositionM=BasePositionM;
                Tower->SetArmClosure(1);
                SetPhase(ERecoveryPhase::Captured,TEXT("Both fittings resting on rails / engines OFF / free rigid body"));
                ActualThrustN=0; ActiveEngines=0;
            }
        }
    }
    ApplyAerodynamics(TargetUp,Dt);
    ApplyThrust(ForceAccel,TargetUp,Dt);
    if(Phase==ERecoveryPhase::Coast || Phase==ERecoveryPhase::Entry)
    {
        if(ActualThrustN<1) UnpoweredSeconds+=Dt;
        else if(PhaseTime>2) bUnpoweredViolation=true;
    }
    if(PropellantKg<=0 && Phase!=ERecoveryPhase::Captured)
    { SetPhase(ERecoveryPhase::Aborted,TEXT("Main propellant exhausted")); ActiveEngines=0; ActualThrustN=0; WriteResult(false,StatusMessage); }
}
void ASuperHeavyRecoveryDirector::ApplyAerodynamics(const FVector& TargetUp,double Dt)
{
    const FQuat Q=Body->GetComponentQuat();
    const FVector Rel=VelocityMps-WindAt(AltitudeM);
    const FVector LocalAir=Q.UnrotateVector(Rel);
    const double Speed=FMath::Max(1.,LocalAir.Size());
    const double BaseCd=LocalAir.Z<0 ? RuntimeProfile->TailFirstDragCoefficient : RuntimeProfile->AxialDragCoefficient;
    const double Cd=BaseCd*(1+0.2*FMath::Exp(-FMath::Square((Mach-1)/0.3)));
    const double FinCdA=3*RuntimeProfile->GridFinAreaM2*RuntimeProfile->GridFinDragCoefficient*FMath::Square(LocalAir.Z/Speed);
    const FVector Drag=-DynamicPressurePa*(RuntimeProfile->DragAreaM2*Cd+FinCdA)*Rel/Speed;
    const FVector SideLocal=-DynamicPressurePa*RuntimeProfile->BodySideAreaM2*RuntimeProfile->BodyNormalCoefficient*FVector(LocalAir.X,LocalAir.Y,0)/Speed;
    AeroForceN=Drag+Q.RotateVector(SideLocal);
    ApplyVehicleForce(ERecoveryForceKind::Aerodynamic,0,AeroForceN,Body->GetCenterOfMass());
    // Three independent tangential fin forces at measured mesh locations.
    // Allocation solves pitch, yaw and roll moments before deflection/rate saturation.
    FVector Torque=AttitudeTorque(TargetUp,0.45,1.35);
    if(ActiveEngines>0) Torque*=0.1; // gimbals take primary authority while powered
    const double Lever=64.4486-Q.UnrotateVector(Body->GetCenterOfMass()/100.-BasePositionM).Z,Radius=6.2;
    const double F3=-Torque.Y/Lever;
    const double Sum=Torque.Z/Radius-F3;
    const FVector Demand((Sum-Torque.X/Lever)/2,(Sum+Torque.X/Lever)/2,F3);
    const double PerRad=DynamicPressurePa*RuntimeProfile->GridFinAreaM2*RuntimeProfile->GridFinLiftSlope;
    const double MaxAngle=RuntimeProfile->GridFinMaxAngleDeg;
    for(int I=0;I<3;++I)
    {
        const double Desired=bContactShutdown || Phase>=ERecoveryPhase::Captured || DynamicPressurePa<100 || Phase<=ERecoveryPhase::Ascent ? 0 : FMath::Clamp(FMath::RadiansToDegrees(Demand[I]/FMath::Max(1.,PerRad)),-MaxAngle,MaxAngle);
        if(I==Experiment.JammedFin)GridFinAnglesDeg[I]=Experiment.JammedFinAngleDeg;
        else GridFinAnglesDeg[I]+=FMath::Clamp(Desired-GridFinAnglesDeg[I],-RuntimeProfile->GridFinRateDegS*Dt,RuntimeProfile->GridFinRateDegS*Dt);
    }
    GridFinAuthority=FMath::Clamp(DynamicPressurePa/1500.,0.,1.);
    if(ActiveEngines==0 && DynamicPressurePa>200 && GridFinAnglesDeg.Size()>0.1) FinControlSeconds+=Dt;
    const FVector Forces=GridFinAnglesDeg*(PerRad*PI/180.);
    const FVector Positions[]={FVector(Radius,0,Lever),FVector(-Radius,0,Lever),FVector(0,Radius,Lever)};
    const FVector Directions[]={FVector(0,1,0),FVector(0,-1,0),FVector(-1,0,0)};
    for(int I=0;I<3;++I)
        ApplyVehicleForce(ERecoveryForceKind::GridFin,I,Q.RotateVector(Directions[I]*Forces[I]),Body->GetCenterOfMass()+Q.RotateVector(Positions[I])*100);
    ApplyReactionControl(Torque,Dt);
    // Inverse-square gravity correction relative to the world's constant -980 cm/s².
    const FVector Down=(FlightGeometry::EarthCenterCm()-Body->GetCenterOfMass()).GetSafeNormal();
    Body->AddForce((Down*Gravity-FVector(0,0,GetWorld()->GetGravityZ()/100.))*MassKg*100);
    // Report total gravity, including Chaos' world-gravity contribution.
    AppliedForces.Add({ERecoveryForceKind::Gravity,0,Body->GetCenterOfMass(),Down*Gravity*MassKg});
}

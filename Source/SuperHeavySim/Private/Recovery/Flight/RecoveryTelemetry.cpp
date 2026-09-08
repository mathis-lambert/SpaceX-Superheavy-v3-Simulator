#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Tests/RecoveryDiagnosticsComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Flight/RecoveryMassProperties.h"
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

void ASuperHeavyRecoveryDirector::UpdateMass()
{
    const auto Properties=RecoveryMass::Booster(*RuntimeProfile,PropellantKg,RcsPropellantKg,!bSeparated);
    MassKg=Properties.MassKg;
    RecoveryMass::Apply(*Body,Properties,BaseOffsetM);
}
FVector ASuperHeavyRecoveryDirector::WindAt(double Height) const
{
    // Boundary layer + diminishing upper atmosphere winds. Deterministic and configurable.
    return RuntimeProfile->WindVelocityMps*Experiment.WindScale*(0.4+0.6*FMath::Clamp(Height/100.,0.,1.))*FMath::Exp(-FMath::Max(0.,Height-10000.)/18000.);
}
void ASuperHeavyRecoveryDirector::UpdateNavigation()
{
    if(!Body) return;
    BasePositionM=Body->GetComponentLocation()/100-Body->GetUpVector()*BaseOffsetM;
    VelocityMps=Body->GetPhysicsLinearVelocity()/100;
    AltitudeM=FlightGeometry::AltitudeM(BasePositionM*100);
    const FVector LocalUp=(BasePositionM*100-FlightGeometry::EarthCenterCm()).GetSafeNormal();
    VerticalSpeedMps=FVector::DotProduct(VelocityMps,LocalUp);
    HorizontalErrorM=FVector2D(BasePositionM-CaptureWorldM).Size();
    TiltDeg=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Body->GetUpVector(),LocalUp),-1.,1.)));
    const auto Air=RecoveryAtmosphere::Sample(AltitudeM,RuntimeProfile->SeaLevelTemperatureOffsetK);
    DensityKgM3=Air.Density; PressurePa=Air.Pressure; Gravity=Air.Gravity;
    const FVector RelativeAir=VelocityMps-WindAt(AltitudeM);
    DynamicPressurePa=0.5*DensityKgM3*RelativeAir.SizeSquared();
    Mach=RelativeAir.Size()/Air.SoundSpeed;
    EngineIspS=FMath::Lerp(RuntimeProfile->SpecificImpulseVacuumS,RuntimeProfile->SpecificImpulseSeaLevelS,FMath::Clamp(PressurePa/101325.,0.,1.));
    const FVector Heading=Tower->GetActorQuat().UnrotateVector(Body->GetForwardVector());
    HeadingErrorDeg=FMath::Abs(FMath::FindDeltaAngleDegrees(FMath::RadiansToDegrees(FMath::Atan2(Heading.Y,Heading.X)),RuntimeProfile->CaptureHeadingDeg));
    SupportContactCount=int32(MissionTime-LastSupportContact[0]<0.2)+int32(MissionTime-LastSupportContact[1]<0.2);
    // Compare both measured fittings with their independent tower support points.
    const FQuat CaptureQ=Tower->GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(RuntimeProfile->CaptureHeadingDeg));
    const FVector L1=BasePositionM+Body->GetComponentQuat().RotateVector(RuntimeProfile->CatchLugPlusM);
    const FVector L2=BasePositionM+Body->GetComponentQuat().RotateVector(RuntimeProfile->CatchLugMinusM);
    CatchLugErrorM=FMath::Max((L1-(CaptureWorldM+CaptureQ.RotateVector(RuntimeProfile->CatchLugPlusM))).Size(),(L2-(CaptureWorldM+CaptureQ.RotateVector(RuntimeProfile->CatchLugMinusM))).Size());
    const double Decel=FMath::Min(RuntimeProfile->LandingDecelerationMps2,13*RuntimeProfile->EngineThrustN/FMath::Max(1.,MassKg)-Gravity);
    BrakingDistanceM=FMath::Square(FMath::Min(VelocityMps.Z,0.))/(2*FMath::Max(0.1,Decel));
    PeakDynamicPressurePa=FMath::Max(PeakDynamicPressurePa,DynamicPressurePa);
    PeakSpeedMps=FMath::Max(PeakSpeedMps,VelocityMps.Size());
    PeakDownrangeM=FMath::Max(PeakDownrangeM,FVector2D(BasePositionM-LaunchWorldM).Size());
}
void ASuperHeavyRecoveryDirector::PredictBallistic()
{
    // Forward point-mass coast prediction, no engine thrust. Recomputed from measured state.
    FVector P=BasePositionM,V=VelocityMps;
    const double Floor=CaptureWorldM.Z+30;
    double T=0;
    for(;T<500 && (P.Z>Floor || V.Z>0);T+=0.75)
    {
        const double H=FlightGeometry::AltitudeM(P*100);
        const auto Air=RecoveryAtmosphere::Sample(H,RuntimeProfile->SeaLevelTemperatureOffsetK);
        const FVector Rel=V-WindAt(H);
        const double Cd=RuntimeProfile->TailFirstDragCoefficient*(1+0.2*FMath::Exp(-FMath::Square((Rel.Size()/Air.SoundSpeed-1)/0.3)));
        const double CdA=RuntimeProfile->DragAreaM2*Cd+3*RuntimeProfile->GridFinAreaM2*RuntimeProfile->GridFinDragCoefficient;
        const FVector A=(FlightGeometry::EarthCenterCm()-P*100).GetSafeNormal()*Air.Gravity-0.5*Air.Density*CdA*Rel.Size()*Rel/FMath::Max(1.,MassKg);
        P+=V*0.75+A*(0.5*0.75*0.75); V+=A*0.75;
    }
    TimeToImpactS=FMath::Max(1.,T);
    PredictedImpactM=P+FVector(V.X,V.Y,0)*RuntimeProfile->LandingDriftCorrectionS;
    PredictedMissM=FVector2D(PredictedImpactM-CaptureWorldM).Size();
}
void ASuperHeavyRecoveryDirector::WriteResult(bool bSuccess,const FString& Reason)
{
    if(bResultWritten) return;
    bResultWritten=true;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Recovery");
    IFileManager::Get().MakeDirectory(*Dir,true);
    const FString FileName=ReportName.IsEmpty() ? ScenarioName : FPaths::MakeValidFileName(ReportName);
    FFileHelper::SaveStringToFile(Csv,*(Dir/FileName+TEXT(".csv")));
    TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"),bSuccess);
    Result->SetStringField(TEXT("scenario"),ScenarioName); Result->SetStringField(TEXT("reason"),Reason);
    Result->SetNumberField(TEXT("duration_s"),MissionTime);
    Result->SetNumberField(TEXT("peak_altitude_m"),PeakAltitudeM);
    Result->SetNumberField(TEXT("peak_tilt_deg"),PeakTiltDeg);
    Result->SetNumberField(TEXT("capture_error_m"),CaptureErrorAtLatch);
    Result->SetNumberField(TEXT("capture_speed_mps"),CaptureSpeedAtLatch);
    Result->SetNumberField(TEXT("capture_tilt_deg"),CaptureTiltAtLatch);
    Result->SetNumberField(TEXT("final_error_m"),HorizontalErrorM);
    Result->SetNumberField(TEXT("final_altitude_m"),AltitudeM);
    Result->SetNumberField(TEXT("final_speed_mps"),VelocityMps.Size());
    Result->SetNumberField(TEXT("restraint_drift_m"),(BasePositionM-LatchPositionM).Size());
    Result->SetNumberField(TEXT("mass_kg"),Body ? Body->GetMass() : 0);
    Result->SetNumberField(TEXT("launch_mass_kg"),InitialMassKg);
    Result->SetNumberField(TEXT("separation_mass_kg"),SeparationMassKg);
    Result->SetNumberField(TEXT("propellant_remaining_kg"),PropellantKg);
    Result->SetNumberField(TEXT("main_propellant_consumed_kg"),MainFuelConsumedKg);
    Result->SetNumberField(TEXT("conditioning_vented_kg"),ConditioningVentedKg);
    Result->SetNumberField(TEXT("ground_supply_kg"),GroundSupplyKg);
    Result->SetNumberField(TEXT("propellant_balance_error_kg"),PropellantKg+MainFuelConsumedKg+ConditioningVentedKg-GroundSupplyKg-RuntimeProfile->PropellantMassKg);
    Result->SetNumberField(TEXT("rcs_propellant_remaining_kg"),RcsPropellantKg);
    Result->SetNumberField(TEXT("boostback_ignition_altitude_m"),BoostbackIgnitionAltitudeM);
    Result->SetNumberField(TEXT("boostback_downrange_m"),BoostbackDownrangeM);
    Result->SetNumberField(TEXT("boostback_seconds"),BoostbackSeconds);
    Result->SetNumberField(TEXT("unpowered_seconds"),UnpoweredSeconds);
    Result->SetNumberField(TEXT("fin_control_seconds"),FinControlSeconds);
    Result->SetNumberField(TEXT("landing_ignition_altitude_m"),LandingIgnitionAltitudeM);
    Result->SetNumberField(TEXT("landing_burn_seconds"),LandingBurnSeconds);
    Result->SetNumberField(TEXT("peak_q_pa"),PeakDynamicPressurePa);
    Result->SetNumberField(TEXT("peak_speed_mps"),PeakSpeedMps);
    Result->SetNumberField(TEXT("peak_downrange_m"),PeakDownrangeM);
    Result->SetNumberField(TEXT("capture_heading_error_deg"),CaptureHeadingAtLatch);
    Result->SetNumberField(TEXT("capture_lug_error_m"),CaptureLugAtLatch);
    Result->SetBoolField(TEXT("unpowered_thrust_violation"),bUnpoweredViolation);
    Result->SetBoolField(TEXT("physical_capture"),true);
    Result->SetStringField(TEXT("dynamics_model"),TEXT("individual-engine-forces-v1"));
    Result->SetNumberField(TEXT("registered_engines"),Engines.Num());
    Result->SetNumberField(TEXT("peak_engine_force_ratio"),PeakEngineForceRatio);
    Result->SetNumberField(TEXT("peak_gimbal_deg"),PeakAppliedGimbalDeg);
    Result->SetNumberField(TEXT("separation_velocity_error_mps"),SeparationVelocityErrorMps);
    Result->SetNumberField(TEXT("separation_momentum_relative_error"),SeparationMomentumRelativeError);
    Result->SetNumberField(TEXT("separation_angular_momentum_relative_error"),SeparationAngularMomentumRelativeError);
    Result->SetNumberField(TEXT("centre_of_mass_from_base_m"),Body->GetComponentQuat().UnrotateVector(Body->GetCenterOfMass()/100.-BasePositionM).Z);
    Result->SetNumberField(TEXT("upper_stage_propellant_kg"),UpperStagePropellantKg);
    Result->SetNumberField(TEXT("upper_stage_fuel_consumed_kg"),UpperStageFuelConsumedKg);
    Result->SetBoolField(TEXT("upper_stage_physical"),bSeparated && GetUpperStageBody() && GetUpperStageBody()->IsSimulatingPhysics());
    Result->SetBoolField(TEXT("launch_hold_released"),bLaunchHoldReleased);
    if(!ContactFixture.IsEmpty()) Result->SetStringField(TEXT("contact_fixture"),ContactFixture);
    Result->SetBoolField(TEXT("left_rail_contact"),EverSupportContact[0]);
    Result->SetBoolField(TEXT("right_rail_contact"),EverSupportContact[1]);
    Result->SetBoolField(TEXT("contact_engine_shutdown"),bContactShutdown);
    Result->SetNumberField(TEXT("left_support_impulse_ns"),SupportImpulseNs.X);
    Result->SetNumberField(TEXT("right_support_impulse_ns"),SupportImpulseNs.Y);
    Result->SetNumberField(TEXT("structural_contacts"),StructuralContactCount);
    Result->SetNumberField(TEXT("launch_catch_axis_distance_m"),FVector2D(LaunchWorldM-CaptureWorldM).Size());
    TArray<TSharedPtr<FJsonValue>> Events;
    for(const FString& Event:PhaseEvents) Events.Add(MakeShared<FJsonValueString>(Event));
    Result->SetArrayField(TEXT("phase_events"),Events);
    FString Json; FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(Dir/FileName+TEXT(".json")));
    UE_LOG(LogRecovery,Display,TEXT("RECOVERY_RESULT %s"),*Json);
    if(bExitAfterTest) FPlatformMisc::RequestExitWithStatus(false,bSuccess ? 0 : 1);
}

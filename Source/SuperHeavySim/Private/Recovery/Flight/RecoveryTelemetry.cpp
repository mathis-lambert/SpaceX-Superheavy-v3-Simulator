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

#include "Recovery/Shared/RecoveryLog.h"

void ASuperHeavyRecoveryDirector::UpdateMass()
{
    const auto Properties=RecoveryMass::Booster(*RuntimeProfile,PropellantKg,RcsPropellantKg,!bSeparated);
    MassKg=Properties.MassKg;
    RecoveryMass::Apply(*Body,Properties,BaseOffsetM);
}
FVector ASuperHeavyRecoveryDirector::WindAt(double Height) const
{
    return RecoveryAtmosphere::WindAt(RuntimeProfile->WindVelocityMps,Height,Experiment.WindScale);
}
void ASuperHeavyRecoveryDirector::UpdateNavigation()
{
    if(!Body)return;
    FRecoveryBodyKinematics Kinematics;
    Kinematics.OriginM=Body->GetComponentLocation()/100.;Kinematics.Rotation=Body->GetComponentQuat();
    Kinematics.VelocityMps=Body->GetPhysicsLinearVelocity()/100.;Kinematics.AngularVelocityWorldRadS=Body->GetPhysicsAngularVelocityInRadians();
    const auto N=RecoveryNavigation::Evaluate(GuidanceConfiguration,Kinematics,PropellantKg,RcsPropellantKg,bSeparated,Experiment);
    BasePositionM=N.BasePositionM;
    VelocityMps=N.VelocityMps;
    AltitudeM=N.AltitudeM;
    VerticalSpeedMps=N.VerticalSpeedMps;
    HorizontalErrorM=N.HorizontalErrorM;
    TiltDeg=N.TiltDeg;
    DensityKgM3=N.DensityKgM3;
    PressurePa=N.PressurePa;
    Gravity=N.Gravity;
    DynamicPressurePa=N.DynamicPressurePa;
    Mach=N.Mach;
    EngineIspS=N.EngineIspS;
    HeadingErrorDeg=N.HeadingErrorDeg;
    CatchLugErrorM=N.CatchLugErrorM;
    BrakingDistanceM=N.BrakingDistanceM;
    SupportContactCount=int32(MissionTime-LastSupportContact[0]<0.2)+int32(MissionTime-LastSupportContact[1]<0.2);
    PeakDynamicPressurePa=FMath::Max(PeakDynamicPressurePa,DynamicPressurePa);
    PeakSpeedMps=FMath::Max(PeakSpeedMps,VelocityMps.Size());
    PeakDownrangeM=FMath::Max(PeakDownrangeM,FVector2D(BasePositionM-LaunchWorldM).Size());
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
    Result->SetNumberField(TEXT("mass_kg"),MassKg);
    Result->SetNumberField(TEXT("dynamics_steps"),DynamicsState.Steps);
    Result->SetNumberField(TEXT("guidance_steps"),GuidanceState.Steps);
    Result->SetNumberField(TEXT("guidance_elapsed_s"),GuidanceState.ElapsedS);
    Result->SetNumberField(TEXT("terminal_planned_s"),GuidanceState.TerminalPlannedSeconds);
    Result->SetNumberField(TEXT("terminal_braking_s"),GuidanceState.TerminalBrakingSeconds);
    Result->SetNumberField(TEXT("terminal_replans"),GuidanceState.TerminalReplans);
    Result->SetNumberField(TEXT("terminal_rejected_plans"),GuidanceState.TerminalRejectedPlans);
    Result->SetNumberField(TEXT("dynamics_time_s"),DynamicsState.ElapsedS);
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
    Result->SetStringField(TEXT("dynamics_model"),TEXT("solver-flight-guidance-v3"));
    Result->SetNumberField(TEXT("registered_engines"),Engines.Num());
    Result->SetNumberField(TEXT("peak_engine_force_ratio"),PeakEngineForceRatio);
    Result->SetNumberField(TEXT("peak_gimbal_deg"),PeakAppliedGimbalDeg);
    Result->SetNumberField(TEXT("separation_velocity_error_mps"),SeparationVelocityErrorMps);
    Result->SetNumberField(TEXT("separation_momentum_relative_error"),SeparationMomentumRelativeError);
    Result->SetNumberField(TEXT("separation_angular_momentum_relative_error"),SeparationAngularMomentumRelativeError);
    Result->SetNumberField(TEXT("centre_of_mass_from_base_m"),DynamicsState.Mass.CentreFromBaseM);
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
    TArray<TSharedPtr<FJsonValue>> Decisions;
    for(const auto& Event:GuidanceState.Events)
    {
        auto Decision=MakeShared<FJsonObject>();
        Decision->SetNumberField(TEXT("phase"),static_cast<uint8>(Event.Phase));
        Decision->SetNumberField(TEXT("reason_code"),static_cast<uint8>(Event.Reason));
        Decision->SetStringField(TEXT("reason"),RecoveryGuidanceReasonText(Event.Reason));
        Decision->SetNumberField(TEXT("time_s"),Event.TimeS);
        Decision->SetNumberField(TEXT("sample_time_s"),Event.SampleTimeS);
        Decision->SetNumberField(TEXT("altitude_m"),Event.AltitudeM);
        Decision->SetNumberField(TEXT("mass_kg"),Event.MassKg);
        Decisions.Add(MakeShared<FJsonValueObject>(Decision));
    }
    Result->SetArrayField(TEXT("guidance_events"),Decisions);
    FString Json; FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(Dir/FileName+TEXT(".json")));
    UE_LOG(LogRecovery,Display,TEXT("RECOVERY_RESULT %s"),*Json);
    if(bExitAfterTest) FPlatformMisc::RequestExitWithStatus(false,bSuccess ? 0 : 1);
}

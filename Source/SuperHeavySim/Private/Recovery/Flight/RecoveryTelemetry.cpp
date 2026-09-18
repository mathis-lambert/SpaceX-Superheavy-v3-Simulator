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
    Result->SetBoolField(TEXT("water_contact"),DynamicsState.bWaterContact);
    Result->SetNumberField(TEXT("submerged_volume_m3"),DynamicsState.SubmergedVolumeM3);
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
    Result->SetNumberField(TEXT("terminal_peak_tracking_error_m"),GuidanceState.TerminalPeakTrackingErrorM);
    Result->SetNumberField(TEXT("front_approach_samples"),GuidanceState.FrontApproachSamples);
    Result->SetNumberField(TEXT("front_min_mast_clearance_m"),GuidanceState.MinimumMastFrontMarginM);
    Result->SetNumberField(TEXT("front_min_corridor_margin_m"),GuidanceState.MinimumFrontCorridorMarginM);
    Result->SetBoolField(TEXT("front_ingress_verified"),GuidanceState.bFrontIngressVerified);
    Result->SetNumberField(TEXT("front_return_offset_m"),GuidanceConfiguration.FrontReturnOffsetM);
    Result->SetNumberField(TEXT("landing_wind_lead_s"),GuidanceConfiguration.LandingWindLeadS);
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
    Result->SetNumberField(TEXT("landing_ignition_time_s"),GuidanceState.LandingIgnitionTimeS);
    Result->SetNumberField(TEXT("landing_ignition_speed_mps"),GuidanceState.LandingIgnitionSpeedMps);
    Result->SetNumberField(TEXT("landing_ignition_mass_kg"),GuidanceState.LandingIgnitionMassKg);
    Result->SetNumberField(TEXT("landing_predicted_distance_m"),GuidanceState.LandingIgnitionDistanceM);
    Result->SetNumberField(TEXT("landing_predicted_fuel_kg"),GuidanceState.LandingIgnitionFuelKg);
    Result->SetNumberField(TEXT("landing_available_thrust_n"),GuidanceState.LandingIgnitionThrustN);
    Result->SetNumberField(TEXT("first_contact_time_s"),GuidanceState.FirstContactTimeS);
    Result->SetNumberField(TEXT("first_contact_speed_mps"),GuidanceState.FirstContactSpeedMps);
    Result->SetNumberField(TEXT("first_contact_vertical_speed_mps"),GuidanceState.FirstContactVerticalSpeedMps);
    Result->SetNumberField(TEXT("first_contact_tilt_deg"),GuidanceState.FirstContactTiltDeg);
    Result->SetNumberField(TEXT("first_contact_angular_speed_deg_s"),GuidanceState.FirstContactAngularSpeedDegS);
    Result->SetNumberField(TEXT("low_slow_approach_seconds"),GuidanceState.LowSlowApproachSeconds);
    Result->SetNumberField(TEXT("peak_q_pa"),PeakDynamicPressurePa);
    Result->SetNumberField(TEXT("peak_speed_mps"),PeakSpeedMps);
    Result->SetNumberField(TEXT("peak_downrange_m"),PeakDownrangeM);
    Result->SetNumberField(TEXT("capture_heading_error_deg"),CaptureHeadingAtLatch);
    Result->SetNumberField(TEXT("capture_lug_error_m"),CaptureLugAtLatch);
    Result->SetBoolField(TEXT("unpowered_thrust_violation"),bUnpoweredViolation);
    Result->SetBoolField(TEXT("physical_capture"),true);
    Result->SetBoolField(TEXT("tower_dynamic"),Tower->IsMechanismDynamic());
    Result->SetNumberField(TEXT("tower_commanded_closure"),Tower->CommandedClosure);
    Result->SetNumberField(TEXT("tower_measured_closure"),Tower->ArmClosure);
    Result->SetNumberField(TEXT("tower_broken_rail_mask"),Tower->BrokenRailMask);
    Result->SetNumberField(TEXT("tower_broken_hinge_mask"),Tower->BrokenHingeMask);
    Result->SetNumberField(TEXT("left_rail_compression_m"),Tower->RailCompressionM.X);
    Result->SetNumberField(TEXT("right_rail_compression_m"),Tower->RailCompressionM.Y);
    Result->SetNumberField(TEXT("left_rail_load_n"),Tower->RailLoadN.X);
    Result->SetNumberField(TEXT("right_rail_load_n"),Tower->RailLoadN.Y);
    Result->SetNumberField(TEXT("left_rail_peak_load_n"),Tower->PeakRailLoadN.X);
    Result->SetNumberField(TEXT("right_rail_peak_load_n"),Tower->PeakRailLoadN.Y);
    Result->SetNumberField(TEXT("left_hinge_peak_torque_nm"),DynamicsState.Tower.PeakHingeTorqueNm.X);
    Result->SetNumberField(TEXT("right_hinge_peak_torque_nm"),DynamicsState.Tower.PeakHingeTorqueNm.Y);
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
    Result->SetNumberField(TEXT("upper_stage_delivered_impulse_ns"),UpperStageState.DeliveredImpulseNs);
    Result->SetNumberField(TEXT("upper_stage_steps"),UpperStageState.Steps);
    Result->SetNumberField(TEXT("upper_stage_elapsed_s"),UpperStageState.ElapsedS);
    Result->SetNumberField(TEXT("upper_stage_propellant_balance_error_kg"),UpperStagePropellantKg+UpperStageFuelConsumedKg-
        (RuntimeProfile->UpperStageMassKg-RuntimeProfile->UpperStageDryMassKg));
    Result->SetBoolField(TEXT("upper_stage_physical"),bSeparated && GetUpperStageBody() && GetUpperStageBody()->IsSimulatingPhysics());
    Result->SetBoolField(TEXT("launch_hold_released"),bLaunchHoldReleased);
    if(!ContactFixture.IsEmpty()) Result->SetStringField(TEXT("contact_fixture"),ContactFixture);
    Result->SetBoolField(TEXT("left_rail_contact"),EverSupportContact[0]);
    Result->SetNumberField(TEXT("solver_support_samples"),DynamicsState.RailSupport.SolverSamples);
    Result->SetNumberField(TEXT("solver_support_mask"),DynamicsState.RailSupport.CurrentMask);
    Result->SetNumberField(TEXT("support_vertical_impulse_ns"),
        DynamicsState.RailSupport.ReactionImpulseWorldNs[0].Z+DynamicsState.RailSupport.ReactionImpulseWorldNs[1].Z);
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
    Result->SetBoolField(TEXT("alternate_recovery"),GuidanceState.bAlternateRecovery);
    Result->SetBoolField(TEXT("estimated_alternate_reachable"),GuidanceState.bSafeAlternateAvailable);
    Result->SetNumberField(TEXT("divert_available_delta_v_mps"),GuidanceState.AvailableDivertDeltaVMps);
    Result->SetNumberField(TEXT("divert_required_delta_v_mps"),GuidanceState.RequiredDivertDeltaVMps);
    Result->SetNumberField(TEXT("corrective_burn_s"),GuidanceState.CorrectiveBurnSeconds);
    Result->SetNumberField(TEXT("rcs_disabled_s"),GuidanceState.RcsDisabledSeconds);
    Result->SetNumberField(TEXT("engine_failed_s"),GuidanceState.EngineFailedSeconds);
    Result->SetNumberField(TEXT("fin_jammed_s"),GuidanceState.FinJammedSeconds);
    Result->SetNumberField(TEXT("rejected_plan_streak_s"),GuidanceState.RejectedPlanSeconds);
    Result->SetNumberField(TEXT("left_rail_load_n"),DynamicsState.Tower.RailLoadN.X);
    Result->SetNumberField(TEXT("right_rail_load_n"),DynamicsState.Tower.RailLoadN.Y);
    Result->SetStringField(TEXT("settled_attitude"),DynamicsState.Body.Rotation.Rotator().ToString());
    FString Json; FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(Dir/FileName+TEXT(".json")));
    UE_LOG(LogRecovery,Display,TEXT("RECOVERY_RESULT %s"),*Json);
    if(bExitAfterTest) FPlatformMisc::RequestExitWithStatus(false,bSuccess ? 0 : 1);
}

void ASuperHeavyRecoveryDirector::InitializeFlightCsv()
{
    Csv=TEXT("time_s,phase,x_m,y_m,base_altitude_m,vx_mps,vy_mps,vz_mps,tilt_deg,target_error_m,throttle,engines,arm_closure,mass_kg,propellant_kg,density_kgm3,q_pa,mach,heading_error_deg,fin_xp_deg,fin_xm_deg,fin_ym_deg,thrust_n,predicted_miss_m,lug_error_m,terminal_replan,terminal_feasible,terminal_horizon_s,terminal_thrust_rejections,terminal_attitude_rejections,terminal_clearance_rejections,terminal_fuel_rejections,plan_px,plan_py,plan_pz,plan_vx,plan_vy,plan_vz,plan_ax,plan_ay,plan_az,plan_ux,plan_uy,plan_uz,plan_tx,plan_ty,plan_tz,plan_wx,plan_wy,plan_wz,plan_mass,plan_fuel,plan_core_thrust,plan_landing_thrust,plan_isp,plan_centre,reference_px,reference_py,reference_pz,reference_vx,reference_vy,reference_vz,terminal_clearance_mask,plan_crossrange_x,plan_crossrange_y,plan_crossrange_z,solver_sample_time_s,solver_x_m,solver_y_m,solver_z_m,solver_vx_mps,solver_vy_mps,solver_vz_mps\n");
}

void ASuperHeavyRecoveryDirector::RecordFlightCsvSample()
{
    Csv+=FString::Printf(TEXT("%.3f,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%.3f,%.3f,%.3f,%.8f,%.3f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f"),MissionTime,*GetPhaseLabel(),BasePositionM.X,BasePositionM.Y,AltitudeM,VelocityMps.X,VelocityMps.Y,VelocityMps.Z,TiltDeg,HorizontalErrorM,Throttle,ActiveEngines,Tower->ArmClosure,MassKg,PropellantKg,DensityKgM3,DynamicPressurePa,Mach,HeadingErrorDeg,GridFinAnglesDeg.X,GridFinAnglesDeg.Y,GridFinAnglesDeg.Z,ActualThrustN,PredictedMissM,CatchLugErrorM);
    const auto& P=GuidanceState.TerminalPlan;
    const auto& I=GuidanceState.TerminalInput;
    Csv+=FString::Printf(TEXT(",%llu,%d,%.6f,%d,%d,%d,%d"),GuidanceState.TerminalReplans,int32(P.bFeasible),P.HorizonS,
        GuidanceState.TerminalCandidate.ThrustRejected,GuidanceState.TerminalCandidate.AttitudeRejected,GuidanceState.TerminalCandidate.ClearanceRejected,GuidanceState.TerminalCandidate.FuelRejected);
    for(const FVector& V:{I.PositionM,I.VelocityMps,I.AccelerationMps2,I.UpWorld,I.TargetM,I.WindMps})
        Csv+=FString::Printf(TEXT(",%.9f,%.9f,%.9f"),V.X,V.Y,V.Z);
    Csv+=FString::Printf(TEXT(",%.9f,%.9f,%.9f,%.9f,%.9f,%.9f"),I.MassKg,I.FuelKg,I.CoreThrustN,I.LandingThrustN,I.IspS,I.CentreFromBaseM);
    const FVector RP=GuidanceState.TerminalReferenceM,RV=GuidanceState.TerminalReferenceVelocityMps;
    Csv+=FString::Printf(TEXT(",%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u,%.6f,%.6f,%.6f"),RP.X,RP.Y,RP.Z,RV.X,RV.Y,RV.Z,GuidanceState.TerminalCandidate.ClearanceReasons,P.CrossrangeCorrectionM.X,P.CrossrangeCorrectionM.Y,P.CrossrangeCorrectionM.Z);
    // Pair physical kinematics with their incoming solver time, not the later
    // actuator endpoint or the interpolated render transform.
    const auto& B=DynamicsState.Body;
    Csv+=FString::Printf(TEXT(",%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f\n"),
        GuidanceState.SampleTimeS,B.OriginM.X,B.OriginM.Y,B.OriginM.Z,B.VelocityMps.X,B.VelocityMps.Y,B.VelocityMps.Z);
}

import unreal as u, json
a=u.EditorAssetLibrary
p=u.load_asset('/Game/Starbase/Data/DA_RecoveryMission')
values=json.loads(r'''{"tail_first_drag_coefficient":1.7,"grid_fin_drag_coefficient":1.2,"boostback_reserve_kg":400000,"landing_drift_correction_s":-8,"landing_ignition_ceiling_m":4500,"apogee_m":95000,"ascent_duration_s":142,"ascent_pitch_deg":58,"ascent_max_acceleration_mps2":25,"timeout_seconds":650,"dry_mass_kg":210000,"propellant_mass_kg":3650000,"upper_stage_mass_kg":1750000,"engine_thrust_n":2451662.5,"specific_impulse_sea_level_s":327,"specific_impulse_vacuum_s":350,"minimum_throttle":0.3,"landing_reserve_kg":75000,"throttle_time_constant":0.25,"max_gimbal_deg":8,"reaction_control_torque_nm":12000000,"reaction_control_propellant_kg":7000,"drag_area_m2":64,"axial_drag_coefficient":0.8,"body_side_area_m2":630,"body_normal_coefficient":1.1,"grid_fin_area_m2":8.2,"grid_fin_lift_slope":2.5,"grid_fin_max_angle_deg":25,"grid_fin_rate_deg_s":45,"max_entry_angle_deg":10,"max_tilt_deg":15,"landing_deceleration_mps2":19,"landing_burn_margin_m":350,"sea_level_temperature_offset_k":8,"capture_radius_m":0.35,"capture_speed_mps":0.6,"capture_tilt_deg":1.5,"capture_heading_tolerance_deg":2,"capture_dwell_seconds":0.6,"capture_heading_deg":90}''')
for key,value in values.items():p.set_editor_property(key,value)
p.set_editor_property('launch_offset_m',u.Vector(140,0,12))
p.set_editor_property('wind_velocity_mps',u.Vector(0,4,0))
p.set_editor_property('catch_lug_plus_m',u.Vector(4.99,0.0079,62.7978))
p.set_editor_property('catch_lug_minus_m',u.Vector(-4.99,0.0079,62.7978))
a.set_metadata_tag(p,'RecoveryFlightVersion','2')
a.save_loaded_asset(p,False)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
for obj in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if obj.get_class().get_name()=='BP_LaunchTower_C':obj.set_editor_property('arm_contact_height_above_base_m',61.9478)
levels.save_current_level()
print('PROFILE_V2_MIGRATED')

"""Read-only asset contract check; run with Unreal's PythonScript commandlet."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u, json, os
root='/Game/Starbase'
out={}
bp=u.load_asset(root+'/Blueprints/BP_LaunchTower')
assert bp and bp.generated_class(), 'Tower Blueprint missing'
tower=u.get_default_object(bp.generated_class())
for property_name in ['architectural_details','left_arm','right_arm']:
    component=tower.get_editor_property(property_name)
    mesh=component.get_editor_property('static_mesh')
    assert mesh and mesh.get_path_name().startswith(root+'/Meshes/'), property_name+' lacks reusable artwork'
    out[property_name]=mesh.get_path_name()
director=u.load_asset(root+'/Blueprints/BP_RecoveryDirector')
assert director and director.generated_class(), 'Director Blueprint missing'
profile=u.load_asset(root+'/Data/DA_RecoveryMission')
assert profile, 'Mission profile missing'
out['mass_components_kg']={key:profile.get_editor_property(key) for key in ['dry_mass_kg','propellant_mass_kg','upper_stage_mass_kg','reaction_control_propellant_kg']}
out['launch_mass_kg']=sum(out['mass_components_kg'].values())
out['apogee_m']=profile.get_editor_property('apogee_m')
assert 80000<=out['apogee_m']<=130000
assert profile.get_editor_property('capture_radius_m')==0.35
out['catch_lugs_m']=[]
for key,sign in [('catch_lug_plus_m',1),('catch_lug_minus_m',-1)]:
    v=profile.get_editor_property(key)
    assert abs(v.x-sign*4.99)<0.001 and abs(v.y-0.0079)<0.001 and abs(v.z-62.7978)<0.001
    out['catch_lugs_m'].append([v.x,v.y,v.z])
assert abs(tower.get_editor_property('arm_contact_height_above_base_m')-61.5678)<0.001
for name in ['SM_ExhaustEnvelope','SM_StarshipDetailed','SM_LaunchMount','Starbase/SM_ServicePickup','Starbase/SM_WindFlag']:
    assert u.load_asset(root+'/Meshes/'+name),name+' missing'
for name in ['M_RaptorPlume','M_VolumetricVapor','M_CryogenicVapor','M_GridFinAlloy','Starbase/M_WindFlag']:
    assert u.load_asset(root+'/Materials/'+name),name+' missing'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level(root+'/Maps/L_RecoveryLab'), 'Map missing'
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
directors=[a for a in actors if a.get_actor_label()=='Recovery_MissionDirector']
assert len(directors)==1, 'Expected exactly one flight director'
assert directors[0].get_editor_property('mission_profile')==profile, 'Scene profile mismatch'
assert directors[0].get_editor_property('tower'), 'Scene tower reference missing'
scene_tower=directors[0].get_editor_property('tower')
assert abs(scene_tower.get_editor_property('arm_contact_height_above_base_m')-61.5678)<0.001
labels=[a.get_actor_label() for a in actors]
for name in ['Recovery_Earth_SM_EarthGlobe','Recovery_Earth_SM_GulfRegion','Recovery_LaunchMount','Recovery_Clouds']:
    assert labels.count(name)==1, name+' missing or duplicated'
out['scene_actor_count']=len(actors);out['success']=True
path=os.path.join(u.Paths.project_saved_dir(),'Recovery','asset-audit.json')
with open(path,'w',encoding='utf-8') as f:json.dump(out,f,indent=2)
u.log('RECOVERY_ASSET_AUDIT '+json.dumps(out))

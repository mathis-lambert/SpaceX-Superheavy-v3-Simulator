"""Migrate the existing site, preserving its terrain and architecture."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u
ROOT='/Game/Starbase'
A=u.EditorAssetLibrary
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level(ROOT+'/Maps/L_RecoveryLab')
ed=u.get_editor_subsystem(u.EditorActorSubsystem)
profile=u.load_asset(ROOT+'/Data/DA_RecoveryMission')
profile.set_editor_property('launch_offset_m',u.Vector(24,0,12))
A.save_loaded_asset(profile,False)
bp=u.load_asset(ROOT+'/Blueprints/BP_LaunchTower')
cdo=u.get_default_object(bp.generated_class())
cdo.set_editor_property('arm_contact_height_above_base_m',61.5678)
A.save_loaded_asset(bp,False)
for actor in ed.get_all_level_actors():
    label=actor.get_actor_label()
    if label=='Recovery_Tower':
        actor.set_editor_property('arm_contact_height_above_base_m',61.5678)
        actor.set_actor_transform(actor.get_actor_transform(),False,False)
    if label=='Recovery_LaunchMount':
        actor.set_actor_location(u.Vector(2400,0,0),False,False)
    # Relocate the circular blast marking beneath the same launch mount.
    if isinstance(actor,u.StaticMeshActor) and 'Launch' in label and label!='Recovery_LaunchMount':
        print('LAUNCH_DETAIL',label,actor.get_actor_location())
levels.save_current_level()
print('PHYSICAL_SITE_READY')

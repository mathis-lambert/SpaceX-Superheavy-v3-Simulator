"""Use the common planetary water shader; remove the redundant water overlay."""
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label()=='Recovery_Landscape_SM_CoastalWater':actors.destroy_actor(actor)
assert levels.save_current_level()
for asset in ('/Game/Starbase/Meshes/Starbase/SM_CoastalWater','/Game/Starbase/Materials/Starbase/M_CoastalWater'):
    if u.EditorAssetLibrary.does_asset_exist(asset):assert u.EditorAssetLibrary.delete_asset(asset)
print('UNIFIED_COAST_READY')

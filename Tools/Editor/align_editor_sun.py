"""Set the saved map sunlight from the same geographic function used at runtime."""
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
environment=u.load_asset('/Game/Starbase/Data/DA_RecoveryEnvironment')
origin=environment.get_editor_property('origin_lat_lon')
sun=next(a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if a.get_actor_label()=='Recovery_Sun')
direction=u.RecoverySkyComponent.calculate_sun_direction(origin.x,origin.y,17.9,-5,252)
sun.set_actor_rotation(u.MathLibrary.make_rot_from_x(direction*-1),False)
sun.light_component.set_intensity(110000)
assert levels.save_current_level()
print('EDITOR_SUN_ALIGNED',direction)

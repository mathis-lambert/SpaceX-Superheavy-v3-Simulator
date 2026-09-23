
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[3]/"Tools"/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
import unreal as u
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Starbase/Maps/L_RecoveryLab')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if not a.get_actor_label().startswith('Recovery_'):print('EXTRA_ACTOR',a.get_actor_label(),a.get_class().get_name())
    if isinstance(a,u.StaticMeshActor):
        c=a.static_mesh_component;b=a.get_actor_bounds(False)
        if max(b[1].x,b[1].y,b[1].z)>100000:print('LARGE_MESH',a.get_actor_label(),c.get_editor_property('static_mesh').get_path_name(),a.get_actor_scale3d(),c.get_editor_property('cast_shadow'))
    if isinstance(a,(u.SkyAtmosphere,u.DirectionalLight,u.SkyLight,u.ExponentialHeightFog)):
        print('ENV_LIGHT',a.get_actor_label(),a.get_actor_location(),a.get_actor_rotation())
        if isinstance(a,u.SkyAtmosphere):
            c=a.get_component_by_class(u.SkyAtmosphereComponent)
            for p in ['transform_mode','bottom_radius','atmosphere_height','rayleigh_scattering','rayleigh_scattering_scale','mie_scattering_scale']:print(p,c.get_editor_property(p))
        if isinstance(a,u.DirectionalLight):
            c=a.light_component
            for p in ['intensity','atmosphere_sun_light','per_pixel_atmosphere_transmittance','light_color','visible']:print(p,c.get_editor_property(p))
        if isinstance(a,u.SkyLight):print('SKY',a.light_component.get_editor_property('intensity'))

"""Inventory the active level and materials before changing continuity settings."""
import json
from pathlib import Path
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
result=[]
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    label=actor.get_actor_label()
    if not any(s in label.lower() for s in ('earth','cloud','atmosphere','fog','starship')):continue
    row=dict(label=label,cls=actor.get_class().get_name(),location=str(actor.get_actor_location()),scale=str(actor.get_actor_scale3d()))
    for cls,properties in ((u.SkyAtmosphereComponent,('bottom_radius','atmosphere_height','transform_mode','mie_scattering_scale','mie_exponential_distribution','aerial_perspective_view_distance_scale')),
                           (u.VolumetricCloudComponent,('planet_radius','layer_bottom_altitude','layer_height','material'))):
        c=actor.get_component_by_class(cls)
        if c:
            row[cls.__name__]={}
            for key in properties:
                try:row[cls.__name__][key]=str(c.get_editor_property(key))
                except Exception:pass
    if isinstance(actor,u.StaticMeshActor):
        c=actor.static_mesh_component
        row['mesh']=c.static_mesh.get_path_name() if c.static_mesh else None
        row['materials']=[m.get_path_name() if m else None for m in c.get_materials()]
    result.append(row)
path=Path(u.Paths.project_saved_dir())/'Recovery/world-layer-inventory.json'
path.write_text(json.dumps(result,indent=2),encoding='utf-8')
print('WORLD_LAYERS_AUDITED',str(path),flush=True)

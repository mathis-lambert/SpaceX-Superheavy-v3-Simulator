"""Read-only inventory for the Starbase scenery/presentation batch."""
import json
from pathlib import Path
import unreal as u

assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Starbase/Maps/L_RecoveryLab')
actors=[]
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    row=dict(label=actor.get_actor_label(),type=actor.get_class().get_name(),position=list(actor.get_actor_location().to_tuple()),scale=list(actor.get_actor_scale3d().to_tuple()))
    if isinstance(actor,u.StaticMeshActor):
        comp=actor.static_mesh_component
        row['mesh']=comp.static_mesh.get_path_name() if comp.static_mesh else None
        row['materials']=[comp.get_material(i).get_path_name() if comp.get_material(i) else None for i in range(comp.get_num_materials())]
    actors.append(row)
path=Path(u.Paths.project_saved_dir())/'Recovery/SitePhotography'
path.mkdir(parents=True,exist_ok=True)
(path/'scene-before.json').write_text(json.dumps(actors,indent=2))
print('SITE_INVENTORY',len(actors))

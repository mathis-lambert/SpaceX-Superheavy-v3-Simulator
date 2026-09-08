"""Read-only asset and scene inspection for the performance pass."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u
from pathlib import Path
import json

root=Path(u.Paths.project_dir())
ed=u.get_editor_subsystem(u.EditorActorSubsystem)
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Starbase/Maps/L_RecoveryLab')
report={'scene':[],'meshes':[],'volumes':[],'cloud':{}}
for a in ed.get_all_level_actors():
    label=a.get_actor_label()
    if any(x in label.lower() for x in ['ground','pad','launch','asphalt','platform','concrete']):
        c=a.get_component_by_class(u.StaticMeshComponent)
        report['scene'].append(dict(name=label,position=str(a.get_actor_location()),bounds=str(a.get_actor_bounds(False)),mesh=c.static_mesh.get_path_name() if c and c.static_mesh else None))
    if label=='Recovery_Clouds':
        c=a.get_component_by_class(u.VolumetricCloudComponent);m=c.get_editor_property('material')
        report['cloud']={'material':m.get_path_name(),'scalar_parameters':{str(n):u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(m,n) for n in u.MaterialEditingLibrary.get_scalar_parameter_names(m)},'texture_parameters':{str(n):str(u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(m,n)) for n in u.MaterialEditingLibrary.get_texture_parameter_names(m)}}
for path in u.EditorAssetLibrary.list_assets('/Game/Starbase/Vehicle/Meshes/Imported_Clean',True,False)+u.EditorAssetLibrary.list_assets('/Game/Starbase/Meshes',True,False):
    m=u.load_asset(path)
    if not isinstance(m,u.StaticMesh):continue
    settings=m.get_editor_property('nanite_settings')
    body=m.get_editor_property('body_setup')
    report['meshes'].append(dict(path=path,nanite=settings.enabled,lods=m.get_num_lods(),triangles=m.get_num_triangles(0),materials=len(m.get_editor_property('static_materials')),collision=str(body.get_editor_property('collision_trace_flag')) if body else None))
registry=u.AssetRegistryHelpers.get_asset_registry()
for data in registry.get_assets_by_class(u.TopLevelAssetPath('/Script/Engine','VolumeTexture'),True):
    report['volumes'].append(str(data.package_name))
(root/'Saved/Recovery/experience-assets-before.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('EXPERIENCE_ASSETS_INSPECTED')

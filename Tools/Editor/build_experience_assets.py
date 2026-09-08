"""Tune generated assets while preserving source meshes and engine content."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from unreal_materials import prop,expression,constant,scalar,vector,custom,connect,material,save,E,A

root=Path(u.Paths.project_dir());report={'removed_overlaps':[],'optimized_meshes':[],'earth_materials':[]}
lamp=material('/Game/Starbase/Materials/M_SiteLamp')
prop(lamp,'shading_model',u.MaterialShadingModel.MSM_UNLIT)
connect(lamp,custom(lamp,{'C':vector(lamp,'Color',(1,.8,.55,1)),'I':scalar(lamp,'Intensity',300)},'return C*I;'),u.MaterialProperty.MP_EMISSIVE_COLOR);save(lamp)

# Keep the authored cloud density/noise. Cache secondary sun visibility using
# cloud shadow maps instead of marching another ray at every primary sample.
original=u.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst')
source=original.get_editor_property('parent')
cloudpath='/Game/Starbase/Materials/M_CloudFlight'
cloud=u.load_asset(cloudpath) if A.does_asset_exist(cloudpath) else A.duplicate_asset(source.get_path_name(),cloudpath)
advanced=[n for n in E.get_material_expressions(cloud) if isinstance(n,u.MaterialExpressionVolumetricAdvancedMaterialOutput)]
assert advanced,'Cloud must expose its volume output'
for node in advanced:
    prop(node,'ray_march_volume_shadow',False)
save(cloud)
instancepath='/Game/Starbase/Materials/MI_CloudFlight'
instance=u.load_asset(instancepath) if A.does_asset_exist(instancepath) else A.duplicate_asset(original.get_path_name(),instancepath)
E.set_material_instance_parent(instance,cloud);A.save_loaded_asset(instance,False)
from unreal_clouds import configure_storm_feature
configure_storm_feature(cloud,instance)
report['cloud_material']=instancepath

# Retain registered aerial detail during suborbital flight; only fade it when
# the entire region is a few screen pixels wide. Spatial feathering hides seams.
for name in ['M_BocaRegion']+[f'M_BocaChica_{i}_{j}' for j in range(4) for i in range(4)]:
    m=u.load_asset('/Game/Starbase/Materials/Earth/'+name)
    for n in E.get_material_expressions(m):
        if not isinstance(n,u.MaterialExpressionCustom):continue
        code=n.get_editor_property('code')
        if n.get_editor_property('description')=='Altitude imagery transition':
            prop(n,'code',code.replace('350000,1600000','18000000,35000000'))
        elif 'float edge=600000-max' in code:
            prop(n,'code',code.replace('edge/60000','smoothstep(0,200000,edge)'))
    save(m);report['earth_materials'].append(name)

meshes=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.StaticMeshEditorSubsystem()
paths=list(A.list_assets('/Game/Starbase/Vehicle/Meshes/Imported_Clean',True,False))
paths += ['/Game/Starbase/Meshes/'+n for n in ['SM_StarshipDetailed','SM_RCSBlock','SM_CaptureArm','SM_TowerServiceCore','SM_LaunchMount']]
for path in paths:
    mesh=u.load_asset(path)
    if not isinstance(mesh,u.StaticMesh):continue
    meshes.remove_collisions(mesh)
    body=mesh.get_editor_property('body_setup')
    if body:prop(body,'collision_trace_flag',u.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    ns=mesh.get_editor_property('nanite_settings');prop(ns,'enabled',True);prop(mesh,'nanite_settings',ns)
    # Render artwork never defines a complex flight collider. Dedicated primitive
    # shapes on the vehicle/tower remain authoritative for the Chaos solver.
    A.save_loaded_asset(mesh,False);report['optimized_meshes'].append(path)

levels=u.get_editor_subsystem(u.LevelEditorSubsystem);levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
ed=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in ed.get_all_level_actors():
    label=a.get_actor_label()
    if label in ['Recovery_LaunchPad','Recovery_CatchPad'] or label.startswith(('Recovery_LaunchRing','Recovery_CatchRing','Recovery_LaunchCross','Recovery_CatchCross')):
        report['removed_overlaps'].append(label);ed.destroy_actor(a);continue
    if label=='Recovery_Clouds':
        c=a.get_component_by_class(u.VolumetricCloudComponent);prop(c,'material',instance)
    if label=='Recovery_Sun':
        c=a.light_component;prop(c,'cast_cloud_shadows',True);prop(c,'cloud_shadow_map_resolution_scale',2.);prop(c,'cloud_shadow_extent',100.)
    if isinstance(a,u.TextRenderActor):
        c=a.get_component_by_class(u.TextRenderComponent);value=str(c.text)
        if 'LZ' in value:c.set_text('LAUNCH & RECOVERY / 01')
levels.save_current_level()
for name,file in [('S_EngineRoar','EngineRoar.wav'),('S_CoastalWind','CoastalWind.wav')]:
    task=u.AssetImportTask();task.filename=str(ART_ROOT/'Audio'/file)
    task.destination_path='/Game/Starbase/Audio';task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=False
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=u.load_asset('/Game/Starbase/Audio/'+name);assert sound
    prop(sound,'looping',True);prop(sound,'volume',1.)
    prop(sound,'virtualization_mode',u.VirtualizationMode.PLAY_WHEN_SILENT)
    A.save_loaded_asset(sound,False)
(root/'Saved/Recovery/experience-assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('EXPERIENCE_ASSETS_READY')

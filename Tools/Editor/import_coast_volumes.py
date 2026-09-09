"""Install measured coastal geometry and the original animated gas volume."""
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from unreal_materials import u,E,A,prop,material,expression,scalar,custom,color,connect,save
from unreal_imports import import_scenery_mesh

root='/Game/Starbase';tools=u.AssetToolsHelpers.get_asset_tools()
svt_path=root+'/FX/Volumes/SVT_TurbulentDeluge'
svt=u.load_asset(svt_path)
factory_path=root+'/FX/Volumes/fluid_noise'
if not svt and A.does_asset_exist(factory_path):
    assert A.rename_asset(factory_path,svt_path)
    svt=u.load_asset(svt_path)
    if A.does_asset_exist(factory_path):A.delete_asset(factory_path)
if not svt:
    task=u.AssetImportTask();task.filename=str(ART_ROOT/'Effects/TurbulentDeluge/Cache/noise/fluid_noise_0001.vdb')
    task.destination_path=root+'/FX/Volumes';task.destination_name='SVT_TurbulentDeluge'
    task.automated=True;task.replace_existing=True;task.save=True;task.factory=u.SparseVolumeTextureFactory()
    tools.import_asset_tasks([task])
    imported=task.get_objects();assert len(imported)==1,imported
    generated_path=imported[0].get_path_name().split('.')[0]
    if generated_path!=svt_path:
        assert A.rename_asset(generated_path,svt_path)
        if A.does_asset_exist(generated_path):A.delete_asset(generated_path)
    svt=u.load_asset(svt_path)
assert svt and svt.get_num_frames()==64,(svt,svt.get_num_frames() if svt else 0)
m=material(root+'/Materials/Effects/M_TurbulentDeluge')
prop(m,'blend_mode',u.BlendMode.BLEND_ADDITIVE);prop(m,'material_domain',u.MaterialDomain.MD_VOLUME)
prop(m,'used_with_heterogeneous_volumes',True)
field=expression(m,u.MaterialExpressionSparseVolumeTextureSampleParameter)
prop(field,'parameter_name','DensityVolume');prop(field,'sparse_volume_texture',svt)
strength=scalar(m,'DensityScale',1.);voxel=scalar(m,'MetersPerVoxel',.15625)
d=custom(m,{'A':(field,'Attributes A'),'S':strength,'V':voxel},'return max(A.r,0)*S*V*3.0;',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m,d,u.MaterialProperty.MP_SUBSURFACE_COLOR)
connect(m,color(m,(.92,.95,.975)),u.MaterialProperty.MP_BASE_COLOR)
connect(m,color(m,(0,0,0)),u.MaterialProperty.MP_EMISSIVE_COLOR);save(m)
report=dict(success=True,volume_frames=svt.get_num_frames(),volume_resolution=[svt.get_size_x(),svt.get_size_y(),svt.get_size_z()],frame_transform=str(svt.get_frame_transform()),terrain=[])
if '-VolumesOnly' not in u.SystemLibrary.get_command_line():
    for j in range(4):
        for i in range(4):
            name=f'SM_BocaChica_{i}_{j}'
            mesh=import_scenery_mesh(ART_ROOT/'Earth/LidarCoast/Meshes'/(name+'.fbx'),root+'/Meshes/Earth/'+name)
            mesh.set_material(0,u.load_asset(root+f'/Materials/Earth/M_BocaChica_{i}_{j}'))
            nanite=mesh.get_editor_property('nanite_settings');prop(nanite,'enabled',True);prop(nanite,'position_precision',2);prop(mesh,'nanite_settings',nanite)
            assert A.save_loaded_asset(mesh,False);report['terrain'].append(name)
            print('COAST_MESH_IMPORTED',name,flush=True)
(SAVED_ROOT/'coast-volumes-assets.json').write_text(json.dumps(report,indent=2))
print('COAST_VOLUMES_IMPORTED',report,flush=True)

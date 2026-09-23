"""Install measured coastal geometry and the original animated gas volume."""
import sys,json,runpy,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from unreal_materials import u,A,prop
from unreal_imports import import_scenery_mesh

root='/Game/Starbase';tools=u.AssetToolsHelpers.get_asset_tools()
svt_path=root+'/FX/Volumes/SVT_TurbulentDeluge'
svt=u.load_asset(svt_path)
source_root=ART_ROOT/'Effects/TurbulentDeluge'
manifest=json.loads((source_root/'sources.json').read_text(encoding='utf-8'))
assert manifest['frames']==64
for record in manifest['files']:
    source=source_root/record['file']
    assert source.is_file() and hashlib.sha256(source.read_bytes()).hexdigest()==record['sha256'],source
source_file=source_root/'Cache/noise/fluid_noise_0001.vdb'
if svt:
    assert u.RecoveryAssetMaintenance.reimport_asset(svt,str(source_file)), 'Volume reimport failed'
    assert A.save_loaded_asset(svt,False)
factory_path=root+'/FX/Volumes/fluid_noise'
if not svt and A.does_asset_exist(factory_path):
    assert A.rename_asset(factory_path,svt_path)
    svt=u.load_asset(svt_path)
    if A.does_asset_exist(factory_path):A.delete_asset(factory_path)
if not svt:
    task=u.AssetImportTask();task.filename=str(source_file)
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
atlas=u.load_asset(root+'/Textures/T_RecoveryVaporAtlas');assert atlas
assert u.RecoveryAssetMaintenance.reimport_asset(atlas,str(ART_ROOT/'Flight/T_RecoveryVaporAtlas.png')), 'Atlas reimport failed'
assert A.save_loaded_asset(atlas,False)
# The canonical builder owns frame interpolation and boundary fading.
# Imports must not recreate the superseded single-frame material.
runpy.run_path(str(Path(__file__).with_name('build_continuous_steam.py')), run_name='__main__')
report=dict(success=True,source_manifest_sha256=hashlib.sha256((source_root/'sources.json').read_bytes()).hexdigest(),atlas_reimported=True,volume_frames=svt.get_num_frames(),volume_resolution=[svt.get_size_x(),svt.get_size_y(),svt.get_size_z()],frame_transform=str(svt.get_frame_transform()),terrain=[])
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

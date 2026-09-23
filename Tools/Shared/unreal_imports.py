"""Shared import policy for static scenery artwork; physical shapes are separate."""
from pathlib import Path
from unreal_materials import u,prop

def import_scenery_mesh(source,asset_path):
    source=Path(source);assert source.is_file(),source
    folder,name=asset_path.rsplit('/',1)
    task=u.AssetImportTask();task.filename=str(source);task.destination_path=folder;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=False
    opts=u.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
    prop(opts,'automated_import_should_detect_type',False);opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
    data=opts.static_mesh_import_data;data.combine_meshes=True;data.generate_lightmap_u_vs=False;data.auto_generate_collision=False
    task.options=opts;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=u.load_asset(asset_path);assert isinstance(mesh,u.StaticMesh),asset_path
    return mesh

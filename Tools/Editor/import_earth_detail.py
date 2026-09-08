"""Replace local imagery sources with verified 4000px NAIP tiles; retain asset references."""
from pathlib import Path
import hashlib
import json
import shutil
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import PROJECT_ROOT,ART_ROOT,SAVED_ROOT
import unreal as u

source=ART_ROOT/'Earth/Detail4000'
records=json.loads((source/'sources.json').read_text(encoding='utf-8'))['tiles']
backup=SAVED_ROOT/'BeforeStrictPhysics/Content/Starbase/Textures/Earth'
backup.mkdir(parents=True,exist_ok=True)
results=[]
for record in records:
    file=source/record['file']
    assert hashlib.sha256(file.read_bytes()).hexdigest()==record['sha256']
    name='T_'+file.stem
    original=PROJECT_ROOT/'Content/Starbase/Textures/Earth'/f'{name}.uasset'
    stored=backup/original.name
    if not stored.exists():shutil.copy2(original,stored)
    assert stored.exists()
    task=u.AssetImportTask()
    task.set_editor_property('filename',str(file))
    task.set_editor_property('destination_path','/Game/Starbase/Textures/Earth')
    task.set_editor_property('destination_name',name)
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',False)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=u.load_asset('/Game/Starbase/Textures/Earth/'+name)
    assert texture is not None
    texture.set_editor_property('power_of_two_mode',u.TexturePowerOfTwoSetting.RESIZE_TO_SPECIFIC_RESOLUTION)
    texture.set_editor_property('resize_during_build_x',4096)
    texture.set_editor_property('resize_during_build_y',4096)
    texture.set_editor_property('max_texture_size',4096)
    texture.set_editor_property('lod_bias',0)
    texture.set_editor_property('srgb',True)
    texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_SHARPEN1)
    texture.set_editor_property('never_stream',False)
    u.EditorAssetLibrary.save_loaded_asset(texture,False)
    results.append(dict(asset=texture.get_path_name(),source_dimensions=record['dimensions'],built_size=[4096,4096],source_sha256=record['sha256'],backup_sha256=hashlib.sha256(stored.read_bytes()).hexdigest()))
(SAVED_ROOT/'earth-detail-import.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
print('EARTH_DETAIL_IMPORTED',len(results))

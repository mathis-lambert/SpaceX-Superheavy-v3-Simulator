"""Persist runtime instancing permutations for the bounded site-detail batches."""
from pathlib import Path
import shutil
import unreal as u

root=Path(u.Paths.project_dir()).resolve()
backup=root/'Saved/Recovery/BeforeControlCleanup/Content/Starbase/Materials'
backup.mkdir(parents=True,exist_ok=True)
for name in ('M_Cladding','M_Graphite','M_Concrete','M_SafetyAmber'):
    path=f'/Game/Starbase/Materials/{name}'
    source=root/f'Content/Starbase/Materials/{name}.uasset'
    target=backup/source.name
    if source.exists() and not target.exists():shutil.copy2(source,target)
    material=u.load_asset(path)
    assert material,path
    material.set_editor_property('used_with_instanced_static_meshes',True)
    u.MaterialEditingLibrary.recompile_material(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material,False),path
    print('SITE_INSTANCING_READY',path)

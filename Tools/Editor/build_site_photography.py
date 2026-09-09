"""Rebuild the site batch in dependency order, preserving a local asset backup."""
from pathlib import Path
import runpy,shutil
root=Path(__file__).resolve().parents[2]
backup=root/'Saved/Recovery/BeforeSitePhotography'
for folder in ('Content/Starbase/Materials','Content/Starbase/Meshes/Earth','Content/Starbase/Maps','Content/Starbase/Data'):
    for source in (root/folder).rglob('*.uasset'):
        destination=backup/source.relative_to(root)
        if not destination.exists():
            destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
    for source in (root/folder).glob('*.umap'):
        destination=backup/source.relative_to(root)
        if not destination.exists():
            destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
for script in ('import_earth_detail.py','build_world_continuity.py','build_site_landscape_assets.py','finalize_site_coast.py','align_editor_sun.py'):
    runpy.run_path(str(Path(__file__).parent/script),run_name='__main__')
print('SITE_PHOTOGRAPHY_BUILD_COMPLETE')

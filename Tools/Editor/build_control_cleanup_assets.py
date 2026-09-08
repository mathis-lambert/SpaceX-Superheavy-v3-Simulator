"""Rebuild only the assets owned by the controls/presentation cleanup."""
from pathlib import Path
for name in ('build_cryogenic_vapor.py','build_site_activity_assets.py','finalize_site_materials.py'):
    source=Path(__file__).parent/name
    exec(compile(source.read_text(encoding='utf-8'),str(source),'exec'),{'__file__':str(source)})
print('CONTROL_CLEANUP_ASSETS_READY')

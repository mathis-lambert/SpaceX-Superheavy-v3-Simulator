"""Rebuild the current visual materials in dependency order, without patchers.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> -nullrhi
-unattended -SCCProvider=None. Raw source generation/import is separate.
"""
from pathlib import Path

directory = Path(__file__).resolve().parent
for name in ('build_surface_materials.py', 'build_world_surfaces.py',
             'build_environment_lighting.py', 'build_layered_weather.py',
             'build_volumetric_vapor.py', 'build_cryogenic_vapor.py',
             'build_propulsion_materials.py', 'build_vapor_trail.py',
             'build_site_activity_assets.py'):
    path = directory / name
    exec(compile(path.read_text(encoding='utf-8'), str(path), 'exec'),
         {'__file__': str(path), '__name__': '__main__'})
print('VISUAL_RENEWAL_READY', flush=True)

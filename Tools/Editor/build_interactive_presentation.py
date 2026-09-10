"""Reproducible presentation pass; run with Unreal closed and commandlet rendering."""
from pathlib import Path
root=Path(__file__).resolve().parent
for name in ('build_world_continuity.py','build_continuous_steam.py','build_cryogenic_vapor.py','build_layered_weather.py','build_world_finish.py','build_starfield_overlay.py'):
    path=root/name
    exec(compile(path.read_text(encoding='utf-8'),str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
print('INTERACTIVE_PRESENTATION_READY',flush=True)

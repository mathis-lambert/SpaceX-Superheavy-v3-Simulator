"""Canonical planet/atmosphere registration, independent of surface authoring."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from unreal_materials import u, prop

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    atmosphere = actor.get_component_by_class(u.SkyAtmosphereComponent)
    if atmosphere:
        # Same 6371 km geocentric frame as terrain/weather. The analytical
        # opaque ground must lie below the globe's finite triangular chords
        # (about 150 m below sea level), otherwise only the denser regional
        # mesh receives sunlight and its rectangular boundary becomes visible.
        prop(atmosphere, 'bottom_radius', 6370.5)
        prop(atmosphere, 'atmosphere_height', 100.5)
        prop(atmosphere, 'trace_sample_count_scale', 2.)
        prop(atmosphere, 'mie_scattering_scale', 1.)
        prop(atmosphere, 'mie_exponential_distribution', 1.2)
    sun = actor.get_component_by_class(u.DirectionalLightComponent)
    if sun and sun.get_editor_property('atmosphere_sun_light'):
        prop(sun, 'per_pixel_atmosphere_transmittance', True)
assert levels.save_current_level()
print('ENVIRONMENT_LIGHTING_READY', flush=True)

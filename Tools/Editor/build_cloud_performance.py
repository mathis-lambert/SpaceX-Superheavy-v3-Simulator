"""Compile out inactive storm work without changing cloud sampling or lighting."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_clouds import u, configure_storm_feature

material = u.load_asset('/Game/Starbase/Materials/M_CloudFlight')
instance = u.load_asset('/Game/Starbase/Materials/MI_CloudFlight')
assert material and instance
enabled = configure_storm_feature(material, instance)
u.log(f'CLOUD_FEATURE storm_shader={enabled}; density, scattering and tracing preserved')

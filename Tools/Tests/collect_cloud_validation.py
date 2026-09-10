"""Preserve cloud comparison evidence without including abandoned tuning runs."""
import json
import shutil
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
saved = root / 'Saved/Recovery'
destination = root / 'Docs/Releases/0.1.0-alpha.9/Clouds'
destination.mkdir(parents=True, exist_ok=True)
before = ['CloudBefore-' + view for view in ('Orbit', 'Globe', 'Layer')]
after = (['CloudStable-' + view for view in ('Ground', 'Layer', 'CloudTop')]
         + ['CloudFlightLOD-' + view for view in ('Orbit', 'Globe')]
         + ['CloudFlightLODStress-Orbit', 'CloudStableStress-CloudTop']
         + ['CloudFlightLODSunset-' + view for view in ('Above', 'Globe')]
         + ['CloudStableSunset-Ground']
         + ['CloudStableNight-Ground'])

# The first baseline capture predates metadata export. Its original command
# line remains in the retained log; describe this provenance explicitly.
baseline_metadata = dict(view='Orbit', camera_altitude_m=94000, pitch_deg=-55,
                         hour=12, weather=2, cloud_mode=3, reconstruction=3,
                         output=[2560, 1440], warmup_seconds=15, capture_frames=360,
                         provenance='Reconstructed from CloudBefore-Orbit.log command line')
for name in before + after:
    log = (saved / (name + '.log')).read_text(encoding='utf-8-sig', errors='replace')
    if 'Fatal error' in log or 'Failed to compile Material' in log:
        raise RuntimeError('Invalid render: ' + name)
    if name in after and 'r.Nanite.ParallelBasePassBuild:0' not in log:
        raise RuntimeError('Stability workaround not verified: ' + name)
    for suffix in ('.csv', '.log'):
        shutil.copy2(saved / (name + suffix), destination / (name + suffix))
    metadata = saved / (name + '.capture.json')
    if metadata.exists():
        shutil.copy2(metadata, destination / metadata.name)
    elif name == 'CloudBefore-Orbit':
        (destination / metadata.name).write_text(json.dumps(baseline_metadata, indent=2))
    else:
        raise RuntimeError('Missing metadata: ' + name)
    shutil.copy2(saved / 'InteractiveAudit' / (name + '.png'), destination / (name + '.png'))

subprocess.run([sys.executable, str(root / 'Tools/Tests/analyze_clouds.py'),
                *[str(destination / (name + '.csv')) for name in before + after],
                '--output', str(destination / 'measurements.json')], check=True)
shutil.copy2(root.parent / 'ArtSource/Weather/manifest.json', destination / 'authored-textures.json')
(destination / 'scope.json').write_text(json.dumps(dict(
    gpu='NVIDIA GeForce RTX 4070 SUPER', cpu='AMD Ryzen 5 9600X',
    engine='UE 5.8.2 / CL 56702186', render_api='D3D12 / SM6', ray_tracing=False,
    frames_per_probe=360, concurrent_rendering_processes=1,
    notes=['Raw frame times include a synchronous screenshot; no samples removed.',
           'Compare the cloud GPU pass first, not a claimed universal flight FPS.',
           'Stress captures use native resolution and actual overcast weather.',
           'Source Editor -game GameFeatureData diagnostic is retained in logs.',
           'Global ocean shading seams are separate from cloud clipping.']), indent=2))
print('CLOUD_EVIDENCE_READY', destination)

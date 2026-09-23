"""Check that before/after pictures describe the same physical moving views.

Image differences are triage measurements, not proof of equivalent visual quality.
The paired original images still need silhouette, plume and horizon inspection.
"""
import argparse
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image


def compare(left, right):
    def frames(directory):
        items = json.loads((directory/'frames.json').read_text(encoding='utf-8-sig'))['frames']
        result = {item['image']: item for item in items}
        if not items or len(result) != len(items):
            raise ValueError(f'Empty or duplicated capture: {directory}')
        return result
    a, b = frames(left), frames(right)
    if a.keys() != b.keys():
        raise ValueError(f'Capture sets differ: {sorted(a.keys() ^ b.keys())}')
    rows = []
    for name in a:
        x, y = a[name], b[name]
        for key in ('phase', 'camera', 'width', 'height', 'solar_hour',
                    'r.ScreenPercentage', 'r.Lumen.HardwareRayTracing'):
            if x[key] != y[key]:
                raise ValueError(f'{name}: unmatched {key}: {x[key]} / {y[key]}')
        for key, tolerance in (('mission_time_s', .001), ('world_time_s', .001),
                               ('altitude_m', .001), ('fov_deg', .0001)):
            if not math.isfinite(x[key]+y[key]) or abs(x[key]-y[key]) > tolerance:
                raise ValueError(f'{name}: unmatched {key}: {x[key]} / {y[key]}')
        errors = {}
        for key, tolerance in (('camera_cm', .1), ('body_cm', .1),
                               ('camera_forward', .000001), ('body_up', .000001)):
            error = math.dist(x[key], y[key])
            if not math.isfinite(error) or error > tolerance:
                raise ValueError(f'{name}: {key} difference {error} exceeds {tolerance}')
            errors[key] = error
        with Image.open(left/name) as im_a, Image.open(right/name) as im_b:
            if im_a.size != (x['width'], x['height']) or im_a.size != im_b.size:
                raise ValueError(f'{name}: unexpected image dimensions')
            pixels_a, pixels_b = np.asarray(im_a.convert('RGB')), np.asarray(im_b.convert('RGB'))
            delta = np.abs(pixels_a.astype(np.float32)-pixels_b.astype(np.float32))
        rows.append({'image': name, 'geometry_errors': errors,
                     'rgb_absolute_difference_mean_255': float(delta.mean()),
                     'rgb_absolute_difference_p99_255': float(np.percentile(delta, 99))})
    return {'matching_views': True, 'visual_review_required': True, 'frames': rows,
            'left_cloud_mode': next(iter(a.values()))['r.VolumetricRenderTarget.Mode'],
            'right_cloud_mode': next(iter(b.values()))['r.VolumetricRenderTarget.Mode']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('left', type=Path)
    parser.add_argument('right', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = compare(args.left, args.right)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False), encoding='utf-8')
    print(f"Matching physical views: {len(result['frames'])}. Visual quality review still required.")

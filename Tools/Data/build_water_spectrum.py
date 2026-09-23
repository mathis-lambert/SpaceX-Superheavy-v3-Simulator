"""Original periodic ocean slope field; deterministic, offline Fourier synthesis.

Broad wind-directed spectrum, not a handful of intersecting sine waves.
Mip averaging converges to a flat normal; no runtime FFT or fluid simulation.
"""
import hashlib
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from project_paths import ART_ROOT

n, extent = 1024, 96.0
k = np.fft.fftfreq(n, d=extent / n) * (2 * np.pi)
kx, ky = np.meshgrid(k, k)
km = np.maximum(np.hypot(kx, ky), 1e-6)
direction = (kx * np.cos(.35) + ky * np.sin(.35)) / km
power = (.15 + .85 * direction ** 2) / km ** 4
power *= np.exp(-(2 * np.pi / 18 / km) ** 4) * np.exp(-(km * .06) ** 2)
power[0, 0] = 0
white = np.random.default_rng(2033).normal(size=(n, n))
height = np.fft.fft2(white) * np.sqrt(power)
sx, sy = (np.fft.ifft2(1j * v * height).real for v in (kx, ky))
scale = .17 / np.sqrt(np.mean(sx * sx + sy * sy))
slopes = np.stack((sx, sy), axis=-1) * scale
assert np.max(np.abs(slopes)) < 1
pixels = np.zeros((n, n, 3), dtype=np.uint8)
pixels[:, :, :2] = np.rint(np.clip(.5 + slopes * .5, 0, 1) * 255).astype(np.uint8)
pixels[:, :, 2] = 128
folder = ART_ROOT / 'Earth/WaterSpectrum'
folder.mkdir(parents=True, exist_ok=True)
path = folder / 'OceanSlopes.png'
Image.fromarray(pixels).save(path)
(folder / 'source.json').write_text(json.dumps(dict(generator=Path(__file__).name,
    seed=2033, extent_m=extent, size=n, rms_slope=.17,
    sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
    license='Original project-generated data'), indent=2) + '\n', encoding='utf-8')
print(path)

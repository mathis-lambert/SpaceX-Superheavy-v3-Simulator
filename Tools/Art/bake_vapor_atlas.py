"""Bake original volumetric smoke impostors; no downloaded artwork required.

RGB contains integrated directional scattering and A Beer-Lambert extinction.
Sixteen independent billows share a padded 4x4 atlas. Runtime particle random
selects a tile, then animated low-amplitude UV advection adds internal motion.
This is an art density field, not a combustion/CFD simulation.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

from pathlib import Path
import numpy as np
from PIL import Image

OUT = ART_ROOT / 'Flight'
OUT.mkdir(parents=True, exist_ok=True)
N, DEPTH, TILE = 320, 80, 512
atlas = Image.new('RGBA', (TILE * 4, TILE * 4))

def noise(x, y, z, grid):
    ix, iy, iz = np.floor(x).astype(np.int32), np.floor(y).astype(np.int32), np.floor(z).astype(np.int32)
    fx, fy, fz = x-ix, y-iy, z-iz
    fx, fy, fz = fx*fx*(3-2*fx), fy*fy*(3-2*fy), fz*fz*(3-2*fz)
    size = grid.shape[0]
    result = np.zeros(np.broadcast_shapes(x.shape, y.shape, z.shape), dtype=np.float32)
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                result += grid[(ix+dx)%size, (iy+dy)%size, (iz+dz)%size] * (fx if dx else 1-fx) * (fy if dy else 1-fy) * (fz if dz else 1-fz)
    return result

axis = np.linspace(-1.25, 1.25, N, dtype=np.float32)
x, y = axis[None, :, None], axis[:, None, None]
z = np.linspace(-1.15, 1.15, DEPTH, dtype=np.float32)[None, None, :]
for tile in range(16):
    rng = np.random.default_rng(4517 + tile)
    field = np.zeros((N, N, DEPTH), dtype=np.float32)
    # A dense core surrounded by overlapping rolled billows of different sizes.
    for j in range(22):
        angle = rng.uniform(0, 2*np.pi)
        radius = rng.uniform(.24, .66) if j else 0
        cx, cy, cz = radius*np.cos(angle), radius*np.sin(angle), rng.uniform(-.3, .3)
        r = rng.uniform(.18, .33) if j else .5
        field = np.maximum(field, np.clip(1-((x-cx)**2+(y-cy)**2+(z-cz)**2)/(r*r), 0, 1))
    grid = rng.random((32, 32, 32), dtype=np.float32)
    turbulent = np.zeros_like(field)
    for frequency, weight in [(3.8,.53),(8.1,.27),(17.7,.14),(37.1,.06)]:
        turbulent += noise(x*frequency+5, y*frequency+9, z*frequency+14, grid)*weight
    density = np.maximum(field - .24 + (turbulent-.5)*1.25, 0) * np.clip(field*6,0,1) * 7
    # Light arrives from upper left: integrate optical depth through the volume.
    optical = np.cumsum(density, axis=0) * (2.5/N) * 1.9
    light = .24 + .76*np.exp(-optical)
    alpha_step = 1-np.exp(-density * (2.3/DEPTH)*2.4)
    transmission = np.ones((N, N), dtype=np.float32)
    radiance = np.zeros((N, N), dtype=np.float32)
    for k in range(DEPTH):
        weight = transmission * alpha_step[:, :, k]
        radiance += weight * light[:, :, k]
        transmission *= 1-alpha_step[:, :, k]
    alpha = 1-transmission
    shade = radiance / np.maximum(alpha, 1e-5)
    rgba = np.stack([shade, shade, shade, alpha], axis=-1)
    im = Image.fromarray(np.uint8(np.clip(rgba, 0, 1)*255), 'RGBA').resize((TILE,TILE), Image.Resampling.LANCZOS)
    atlas.paste(im, ((tile%4)*TILE, (tile//4)*TILE))
    print(f'Vapor volume {tile+1}/16', flush=True)
atlas.save(OUT / 'T_RecoveryVaporAtlas.png')
print(OUT / 'T_RecoveryVaporAtlas.png', flush=True)

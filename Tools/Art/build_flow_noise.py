"""Deterministic periodic 128-cubed density fields packed as a volume atlas.

Independent channels have documented distributions, unlike engine noise assets
whose packed channels are not a stable authoring contract.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from project_paths import ART_ROOT
import numpy as np
from PIL import Image
import json
import hashlib

root = ART_ROOT / 'Effects' / 'FlowNoise'
root.mkdir(parents=True, exist_ok=True)
rng = np.random.default_rng(8102026)
channels = []
for sigma in (7., 3., 1.2):
    f=np.fft.fftfreq(128);fz=np.fft.rfftfreq(128)
    radius=f[:,None,None]**2+f[None,:,None]**2+fz[None,None,:]**2
    spectrum=np.fft.rfftn(rng.standard_normal((128,128,128)))
    n=np.fft.irfftn(spectrum*np.exp(-2*np.pi**2*sigma**2*radius),s=(128,128,128),axes=(0,1,2)).astype(np.float32)
    n = np.clip(.5+n/max(float(n.std()),1.e-8)*.18, 0, 1)
    channels.append((n*255).astype(np.uint8))
volume = np.stack(channels + [np.full_like(channels[0],255)], axis=-1)
atlas = np.zeros((1024,2048,4),np.uint8)
for z in range(128):
    atlas[(z//16)*128:(z//16+1)*128,(z%16)*128:(z%16+1)*128] = volume[z]
path=root/'FlowNoise128.png'
Image.fromarray(atlas).save(path)
(root/'manifest.json').write_text(json.dumps(dict(seed=8102026,dimensions=[128]*3,sigma_voxels=[7,3,1.2],
    mean=.5,stddev=.18,license='Project-authored procedural field',sha256=hashlib.sha256(path.read_bytes()).hexdigest()),indent=2))
print('FLOW_NOISE_READY',path)

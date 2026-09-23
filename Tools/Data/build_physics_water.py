"""Pack CPU water classification from the same masks used by world shading."""
import json
import math
import struct
import hashlib
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT.parent / 'ArtSource/Earth'
half_lat = math.degrees(6000 / 6371000)
half_lon = half_lat / math.cos(math.radians(25.9973))
layers = [
    (ART / 'Continuity/CoastalHydrology.png', 1024, 1024,
     (-97.1569-half_lon, 25.9973-half_lat, -97.1569+half_lon, 25.9973+half_lat)),
    (ART / 'WaterCoverage/RegionalOcean.png', 1024, 1024, (-109.1569, 13.9973, -85.1569, 37.9973)),
    (ART / 'WaterCoverage/GlobalOcean.png', 2048, 1024, (-180, -90, 180, 90)),
]
out = ROOT / 'Content/Starbase/Data/Water/Surface.bin'
out.parent.mkdir(parents=True, exist_ok=True)
sources = []
with out.open('wb') as stream:
    stream.write(struct.pack('<II', 0x31525457, len(layers)))
    for path, width, height, bounds in layers:
        source = Image.open(path)
        red = source.split()[0].resize((width, height), Image.Resampling.BOX)
        bits = np.packbits(np.asarray(red).reshape(-1) >= 128, bitorder='little').tobytes()
        stream.write(struct.pack('<II4d', width, height, *bounds))
        stream.write(bits)
        sources.append({'source': path.relative_to(ART).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'size': [width, height], 'bounds': bounds})
manifest = {'output': out.relative_to(ROOT).as_posix(), 'sha256': hashlib.sha256(out.read_bytes()).hexdigest(), 'sources': sources,
            'limitations': 'Closed hull approximation; local classification about 12 m, coarser away from Starbase. No structural breakup or flooding.'}
(ROOT/'Docs/Credits/PHYSICS_WATER_SOURCES.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
print(out, out.stat().st_size)

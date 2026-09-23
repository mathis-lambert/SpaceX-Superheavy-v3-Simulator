"""Cache attributable Sentinel-2 coast mosaics; never upscale to invent detail.

EOX 2016 is CC BY 4.0 and is used for geography, not present-day site layout.
USGS NAIP remains the near-site layer. Each request retains its exact bbox/hash.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from project_paths import ART_ROOT
import hashlib
import json
import time
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from concurrent.futures import ThreadPoolExecutor
from PIL import Image

ROOT = ART_ROOT / 'Earth' / 'Continuity'
ROOT.mkdir(parents=True, exist_ok=True)
SERVICE = 'https://tiles.maps.eox.at/wms'
LAYER = 's2cloudless'
CAP = SERVICE + '?service=WMS&request=GetCapabilities'
raw = urllib.request.urlopen(CAP, timeout=60).read()
(ROOT / 'eox-capabilities.xml').write_bytes(raw)
tree = ET.fromstring(raw)
layers = [el for el in tree.iter() if el.tag.split('}')[-1] == 'Layer']
selected = next(el for el in layers if any(ch.tag.split('}')[-1] == 'Name' and ch.text == LAYER for ch in el))
title = next(ch.text for ch in selected if ch.tag.split('}')[-1] == 'Title')
assert '2016' in title, title
LAT, LON = 25.9973, -97.1569
jobs = []
layers_to_fetch = [('Regional', .3)] if '--regional' in sys.argv else [('Coast', 1.2), ('Gulf', 12.)]
for name, half in layers_to_fetch:
    for y in range(4):
        for x in range(4):
            bbox = [LON-half+x*half/2, LAT+half-(y+1)*half/2, LON-half+(x+1)*half/2, LAT+half-y*half/2]
            args = dict(service='WMS', request='GetMap', version='1.1.1', layers=LAYER,
                        styles='', srs='EPSG:4326', bbox=','.join(map(str, bbox)), width=2048, height=2048, format='image/png')
            jobs.append(dict(name=name, x=x, y=y, bbox=bbox, file=f'{name}_{x}_{y}.png', url=SERVICE+'?'+urllib.parse.urlencode(args)))

def download(job):
    path = ROOT / job['file']
    if not path.exists():
        for attempt in range(3):
            try:
                data = urllib.request.urlopen(job['url'], timeout=90).read()
                import io
                with Image.open(io.BytesIO(data)) as im:
                    assert im.size == (2048, 2048), im.size
                    im.verify()
                path.write_bytes(data)
                break
            except Exception:
                if attempt == 2:
                    raise
                time.sleep(2)
    with Image.open(path) as im:
        assert im.size == (2048, 2048)
    job['sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
    print(job['file'], flush=True)
    return job

with ThreadPoolExecutor(max_workers=3) as pool:
    sources = list(pool.map(download, jobs))
for name, half in layers_to_fetch:
    mosaic = Image.new('RGB', (8192, 8192))
    for j in sources:
        if j['name'] == name:
            with Image.open(ROOT / j['file']) as im:
                mosaic.paste(im.convert('RGB'), (j['x']*2048, j['y']*2048))
    mosaic.save(ROOT / f'{name}8192.png')
manifest = dict(layer=LAYER, title=title, year=2016, license='CC BY 4.0',
    license_url='https://cloudless.eox.at/pricing',
    attribution='Data & Viewing Products: EOxCloudless https://cloudless.eox.at by EOX IT Services GmbH (Contains modified Copernicus Sentinel data 2016)',
    note='Geographic mosaic; does not document current Starbase buildings. WMS output sampling is not a claim of sensor resolution.',
    sources=sources)
(ROOT / ('regional-sources.json' if '--regional' in sys.argv else 'sources.json')).write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print('WORLD_CONTINUITY_SOURCES_READY', flush=True)

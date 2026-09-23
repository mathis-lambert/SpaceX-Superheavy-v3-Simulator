"""Cache NASA global Earth imagery with provenance and hashes. Local imagery and elevation have dedicated source pipelines."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import ART_ROOT

import urllib.request, json, hashlib, time
from PIL import Image
Image.MAX_IMAGE_PIXELS=300_000_000
ROOT=ART_ROOT/'Earth'
ROOT.mkdir(parents=True,exist_ok=True)
def fetch(name,url):
    path=ROOT/name
    if not path.exists():
        for attempt in range(4):
            try:
                with urllib.request.urlopen(url,timeout=180) as response: data=response.read()
                path.write_bytes(data)
                with Image.open(path) as im: im.verify()
                break
            except Exception:
                if path.exists(): path.unlink()
                if attempt==3: raise
                time.sleep(3)
    with Image.open(path) as im: size=im.size
    print(name,size,flush=True)
    return dict(file=name,url=url,size=size,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
source=fetch('Earth_September.jpg','https://assets.science.nasa.gov/content/dam/science/esd/eo/images/bmng/bmng-topography-bathymetry/september/world.topo.bathy.200409.3x21600x10800.jpg')
(ROOT/'global-imagery-sources.json').write_text(json.dumps(dict(sources=[source],credits=['NASA Earth Observatory / Blue Marble Next Generation, September 2004']),indent=2),encoding='utf-8')
print('GLOBAL_EARTH_DATA_READY')

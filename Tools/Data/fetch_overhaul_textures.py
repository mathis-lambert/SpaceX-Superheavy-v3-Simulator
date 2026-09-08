"""Higher resolution public-domain NAIP layers; original cache stays intact."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

from pathlib import Path
import urllib.request,urllib.parse,json,hashlib,math,concurrent.futures
from PIL import Image
OUT=ART_ROOT/'VehicleDetails';OUT.mkdir(parents=True,exist_ok=True)
LAT,LON=25.9973,-97.1569;dy=180/(math.pi*6371000);dx=dy/math.cos(math.radians(LAT))
def fetch(item):
    name,bbox=item;p=OUT/name
    params=dict(bbox=','.join(map(str,bbox)),bboxSR=4326,imageSR=4326,size='4000,4000',format='png32',f='image',bandIds='0,1,2')
    url='https://imagery.nationalmap.gov/arcgis/rest/services/USGSNAIPImagery/ImageServer/exportImage?'+urllib.parse.urlencode(params)
    if not p.exists():
        with urllib.request.urlopen(url,timeout=150) as r:data=r.read()
        p.write_bytes(data)
    with Image.open(p) as im:assert im.size==(4000,4000)
    print(name,flush=True);return dict(file=name,url=url,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
jobs=[]
for j in range(4):
    for i in range(4):
        x,y=-6000+i*3000,-6000+j*3000;jobs.append((f'BocaChica_{i}_{j}.png',[LON+x*dx,LAT+y*dy,LON+(x+3000)*dx,LAT+(y+3000)*dy]))
for j in range(2):
    for i in range(2):
        x,y=-60000+i*60000,-60000+j*60000;jobs.append((f'Basin_{i}_{j}.png',[LON+x*dx,LAT+y*dy,LON+(x+60000)*dx,LAT+(y+60000)*dy]))
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:results=list(pool.map(fetch,jobs))
canvas=Image.new('RGBA',(8000,8000))
for j in range(2):
    for i in range(2):
        with Image.open(OUT/f'Basin_{i}_{j}.png') as im:canvas.paste(im,(i*4000,(1-j)*4000))
canvas.save(OUT/'BocaRegionHi.png')
(OUT/'sources.json').write_text(json.dumps(dict(credit='USGS / USDA NAIP, historic public-domain orthoimagery',sources=results),indent=2))
print('OVERHAUL_TEXTURES_READY',flush=True)

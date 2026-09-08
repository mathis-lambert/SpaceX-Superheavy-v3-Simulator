"""Cache public NASA / USGS geographic source data, with provenance and hashes."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

from pathlib import Path
import urllib.request, urllib.parse, json, math, hashlib, concurrent.futures, time
from PIL import Image
import numpy as np
Image.MAX_IMAGE_PIXELS=300_000_000
ROOT=ART_ROOT/'Earth'
ROOT.mkdir(parents=True,exist_ok=True)
LAT,LON=25.9973,-97.1569
DLAT=180/(math.pi*6371000)
DLON=DLAT/math.cos(math.radians(LAT))
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
def export(service,bbox,size,**kw):
    params=dict(bbox=','.join(map(str,bbox)),bboxSR=4326,imageSR=4326,size=f'{size},{size}',format='png32',f='image',**kw)
    return service+'/exportImage?'+urllib.parse.urlencode(params)
jobs=[('Earth_September.jpg','https://assets.science.nasa.gov/content/dam/science/esd/eo/images/bmng/bmng-topography-bathymetry/september/world.topo.bathy.200409.3x21600x10800.jpg')]
tiles=[]
for j in range(4):
    for i in range(4):
        x,y=-6000+i*3000,-6000+j*3000
        bbox=[LON+x*DLON,LAT+y*DLAT,LON+(x+3000)*DLON,LAT+(y+3000)*DLAT]
        name=f'BocaChica_{i}_{j}.png'
        jobs.append((name,export('https://imagery.nationalmap.gov/arcgis/rest/services/USGSNAIPImagery/ImageServer',bbox,2048,bandIds='0,1,2')))
        tiles.append(dict(name=name,bbox=bbox,x=x,y=y))
bbox=[LON-6000*DLON,LAT-6000*DLAT,LON+6000*DLON,LAT+6000*DLAT]
dem=export('https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer',bbox,1024,pixelType='F32',renderingRule=json.dumps({'rasterFunction':'None'})).replace('format=png32','format=tiff')
jobs.append(('BocaChica_DEM.tif',dem))
regional_bbox=[LON-60000*DLON,LAT-60000*DLAT,LON+60000*DLON,LAT+60000*DLAT]
jobs.append(('BocaRegion.png',export('https://imagery.nationalmap.gov/arcgis/rest/services/USGSNAIPImagery/ImageServer',regional_bbox,4000,bandIds='0,1,2')))
params=dict(service='WMS',request='GetMap',version='1.1.1',layers='BlueMarble_ShadedRelief_Bathymetry',styles='',srs='EPSG:4326',bbox=f'{LON-12},{LAT-12},{LON+12},{LAT+12}',width=4096,height=4096,format='image/jpeg')
jobs.append(('GulfRegion.jpg','https://gibs.earthdata.nasa.gov/wms/epsg4326/best/wms.cgi?'+urllib.parse.urlencode(params)))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    sources=list(pool.map(lambda item:fetch(*item),jobs))
(ROOT/'sources.json').write_text(json.dumps(dict(origin=[LAT,LON],radius_m=6371000,tiles=tiles,sources=sources,credits=['NASA Earth Observatory / Blue Marble Next Generation, September 2004','NASA GIBS BlueMarble Shaded Relief Bathymetry','USGS / USDA NAIP public domain orthoimagery','USGS 3DEP public domain bare-earth elevation']),indent=2))
np.save(ROOT/'elevation.npy',np.asarray(Image.open(ROOT/'BocaChica_DEM.tif')))
print('EARTH_DATA_READY')

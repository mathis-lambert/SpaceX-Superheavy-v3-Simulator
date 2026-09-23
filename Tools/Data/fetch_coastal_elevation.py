"""Use source 3DEP GeoTIFFs, avoiding an unreliable image-service float export."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
import urllib.request
import hashlib
import json
import math
import numpy as np
from PIL import Image
Image.MAX_IMAGE_PIXELS=300_000_000
root=ART_ROOT/'Earth/Continuity/Elevation';root.mkdir(parents=True,exist_ok=True)
size=2048
lat=25.9973+np.linspace(6000,-6000,size)*180/(math.pi*6371000)
lon=-97.1569+np.linspace(-6000,6000,size)*180/(math.pi*6371000*math.cos(math.radians(25.9973)))
height=np.zeros((size,size),np.float32);valid=np.zeros((size,size),bool);sources=[]
for name in ('n26w098','n27w098'):
    url=f'https://prd-tnm.s3.amazonaws.com/StagedProducts/Elevation/13/TIFF/current/{name}/USGS_13_{name}.tif'
    path=root/f'USGS_13_{name}.tif'
    if not path.exists():
        with urllib.request.urlopen(url,timeout=90) as response, path.with_suffix('.download').open('wb') as output:
            while chunk:=response.read(1024*1024):output.write(chunk)
        path.with_suffix('.download').replace(path)
    with Image.open(path) as im:
        scale=im.tag_v2[33550];tie=im.tag_v2[33922]
        data=np.asarray(im)
        x=(lon-tie[3])/scale[0]-.5;y=(tie[4]-lat)/scale[1]-.5
        ix=np.floor(x).astype(int);iy=np.floor(y).astype(int)
        inside=(iy[:,None]>=0)&(iy[:,None]<data.shape[0]-1)&(ix[None,:]>=0)&(ix[None,:]<data.shape[1]-1)
        a=(x-ix)[None,:];b=(y-iy)[:,None];ix=np.clip(ix,0,data.shape[1]-2);iy=np.clip(iy,0,data.shape[0]-2)
        samples=[data[iy[:,None]+dy,ix[None,:]+dx] for dx,dy in ((0,0),(1,0),(0,1),(1,1))]
        good=inside & np.logical_and.reduce([np.isfinite(v)&(v>-20)&(v<60) for v in samples])
        interpolated=(samples[0]*(1-a)+samples[1]*a)*(1-b)+(samples[2]*(1-a)+samples[3]*a)*b
        height[good]=interpolated[good];valid|=good
        sources.append(dict(url=url,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),dimensions=list(im.size),pixel_scale=list(scale),tiepoint=list(tie)))
        print(name,'valid local coverage',valid.mean(),flush=True)
        del data,samples
np.save(root/'height_m.npy',height);np.save(root/'valid.npy',valid)
result=dict(source='USGS 3DEP / 1/3 arc-second',vertical_datum='NAVD88',runtime_pad_datum_offset_m=4.5,
    output_grid=[size,size],coverage=float(valid.mean()),range_m=np.percentile(height[valid],[0,50,95,100]).tolist(),sources=sources,
    note='Resampling does not increase source resolution. Missing offshore/Mexican elevation uses a documented sea-level fallback.')
(root/'sources.json').write_text(json.dumps(result,indent=2));print('COASTAL_ELEVATION_READY',result['range_m'],flush=True)

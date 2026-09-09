"""Fetch genuine 4000px NAIP source tiles; keep the previous imagery untouched."""
import concurrent.futures
import hashlib
import json
import math
from pathlib import Path
import sys
import urllib.parse
import urllib.request

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
from PIL import Image

DEST=ART_ROOT/'Earth'/'Registered4000'
SERVICE='https://imagery.nationalmap.gov/arcgis/rest/services/USGSNAIPImagery/ImageServer'
LAT,LON,RADIUS=25.9973,-97.1569,6371000.

def fetch_tile(tile):
    i,j=tile
    x,y=-6000+i*3000,-6000+j*3000
    dlat=180/(math.pi*RADIUS)
    dlon=dlat/math.cos(math.radians(LAT))
    bbox=[LON+x*dlon,LAT+y*dlat,LON+(x+3000)*dlon,LAT+(y+3000)*dlat]
    parameters=dict(bbox=','.join(map(str,bbox)),bboxSR=4326,imageSR=4326,size='4000,4000',format='png32',bandIds='0,1,2',adjustAspectRatio='false',f='pjson')
    url=SERVICE+'/exportImage?'+urllib.parse.urlencode(parameters)
    path=DEST/f'BocaChica_{i}_{j}.png'
    metadata_path=path.with_suffix('.json')
    if not path.exists() or not metadata_path.exists():
        with urllib.request.urlopen(url,timeout=90) as response:
            metadata=json.load(response)
        assert 'error' not in metadata,metadata
        actual=[metadata['extent'][k] for k in ('xmin','ymin','xmax','ymax')]
        assert all(abs(a-b)<1e-8 for a,b in zip(actual,bbox)),(bbox,actual)
        with urllib.request.urlopen(metadata['href'],timeout=90) as response:data=response.read()
        temporary=path.with_suffix('.download')
        temporary.write_bytes(data)
        with Image.open(temporary) as picture:
            picture.verify()
        temporary.replace(path)
        metadata_path.write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    metadata=json.loads(metadata_path.read_text())
    actual=[metadata['extent'][k] for k in ('xmin','ymin','xmax','ymax')]
    assert all(abs(a-b)<1e-8 for a,b in zip(actual,bbox)),(bbox,actual)
    with Image.open(path) as picture:
        assert picture.size==(4000,4000),picture.size
    return dict(file=path.name,url=url,bbox=bbox,reported_extent=actual,dimensions=[4000,4000],nominal_output_metres_per_pixel=3000/4000,sha256=hashlib.sha256(path.read_bytes()).hexdigest())

def main():
    DEST.mkdir(parents=True,exist_ok=True)
    records=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
        for record in executor.map(fetch_tile,[(i,j) for j in range(4) for i in range(4)]):
            records.append(record)
            print(record['file'],record['dimensions'],flush=True)
    (DEST/'sources.json').write_text(json.dumps(dict(source='USGS/USDA NAIP orthoimagery service',credits='USGS / USDA NAIP public domain imagery',note='Requested output resolution does not prove the native ground sampling distance; verify source metadata before making a resolution claim.',tiles=records),indent=2),encoding='utf-8')
    print('NAIP_DETAIL_READY',len(records),flush=True)

if __name__=='__main__':
    main()

"""Rasterize public-domain coastline polygons, independent of photograph color.

Natural Earth is a cartographic source, not a local hydrographic survey. The
registered 3DEP/LiDAR-derived mask remains authoritative near Starbase.
"""
import hashlib
import json
import struct
import sys
import urllib.request
import zipfile
from pathlib import Path
from PIL import Image, ImageDraw
import numpy as np

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT

ROOT=ART_ROOT/'Earth/WaterCoverage'
URL='https://naturalearth.s3.amazonaws.com/10m_physical/ne_10m_land.zip'


def polygons(data):
    assert struct.unpack_from('>i',data,0)[0]==9994
    assert struct.unpack_from('<i',data,32)[0]==5
    offset=100
    while offset<len(data):
        _,words=struct.unpack_from('>2i',data,offset); offset+=8
        block=data[offset:offset+words*2]; offset+=words*2
        kind=struct.unpack_from('<i',block)[0]
        if kind==0:continue
        assert kind==5
        count,points=struct.unpack_from('<2i',block,36)
        starts=list(struct.unpack_from('<'+str(count)+'i',block,44))+[points]
        xy=np.frombuffer(block,dtype='<f8',count=points*2,offset=44+count*4).reshape(-1,2)
        yield [xy[starts[i]:starts[i+1]] for i in range(count)]


def rasterize(shapes,bounds,size):
    west,south,east,north=bounds; width,height=size
    image=Image.new('L',size,255); draw=ImageDraw.Draw(image)
    for rings in shapes:
        for ring in rings:
            if len(ring)<3:continue
            if ring[:,0].max()<west or ring[:,0].min()>east or ring[:,1].max()<south or ring[:,1].min()>north:continue
            area=np.sum(ring[:-1,0]*ring[1:,1]-ring[1:,0]*ring[:-1,1])
            # ESRI polygon exterior rings are clockwise, interior rings CCW.
            points=[((float(x)-west)/(east-west)*width,(north-float(y))/(north-south)*height) for x,y in ring]
            draw.polygon(points,fill=0 if area<0 else 255)
    return image


def main():
    ROOT.mkdir(parents=True,exist_ok=True)
    source=ROOT/'ne_10m_land.zip'
    if not source.exists():
        with urllib.request.urlopen(URL,timeout=120) as response:data=response.read()
        # Verify before publishing a cache file; never leave a partial download.
        import io
        with zipfile.ZipFile(io.BytesIO(data)) as archive:assert archive.testzip() is None
        source.write_bytes(data)
    with zipfile.ZipFile(source) as archive:
        shapes=list(polygons(archive.read('ne_10m_land.shp')))
    jobs=[('GlobalOcean',(-180,-90,180,90),(8192,4096)),
          ('RegionalOcean',(-109.1569,13.9973,-85.1569,37.9973),(4096,4096))]
    records=[]
    for name,bounds,size in jobs:
        im=rasterize(shapes,bounds,size); path=ROOT/(name+'.png'); im.save(path)
        records.append(dict(file=path.name,bounds=bounds,size=size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    # Independent geographic checks catch inverted winding or raster orientation.
    im=Image.open(ROOT/'GlobalOcean.png')
    # Avoid Natural Earth's cartographic "Null Island" at exactly (0, 0).
    for lon,lat,water in [(-95,25,True),(-100,35,False),(-30,0,True),(20,20,False)]:
        actual=im.getpixel((int((lon+180)/360*im.width),int((90-lat)/180*im.height)))>127
        assert actual==water,(lon,lat,actual)
    manifest=dict(source=URL,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  license='Natural Earth public domain',reference='https://www.naturalearthdata.com/downloads/10m-physical-vectors/10m-land/',
                  limitations='Cartographic coastline; not surveyed tidal or bathymetric data',outputs=records)
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2))
    print(json.dumps(manifest,indent=2))


if __name__=='__main__':main()

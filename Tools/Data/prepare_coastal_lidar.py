"""Cache USGS one-metre DEMs and reproject a registered coastal height field.

The source is a measured bare-earth DEM, not generated landscape. Offshore and
out-of-coverage pixels retain the previous documented 3DEP/sea-level fallback.
"""
import sys, json, hashlib, urllib.request, concurrent.futures
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT, SAVED_ROOT
import numpy as np
import rasterio
from rasterio.warp import reproject, Resampling
from rasterio.transform import from_bounds

ROOT=ART_ROOT/'Earth/LidarCoast'; ROOT.mkdir(parents=True,exist_ok=True)
DISCOVERY='https://tnmaccess.nationalmap.gov/api/v1/products?datasets=Digital%20Elevation%20Model%20%28DEM%29%201%20meter&bbox=-97.218,25.943,-97.096,26.052&max=50&outputFormat=JSON'

def fetch(item):
    path=ROOT/Path(item['downloadURL']).name
    if not path.exists():
        temporary=path.with_suffix('.download')
        with urllib.request.urlopen(item['downloadURL'],timeout=120) as source,temporary.open('wb') as target:
            while chunk:=source.read(1024*1024):target.write(chunk)
        if temporary.stat().st_size!=item['sizeInBytes']:raise ValueError('Incomplete USGS DEM download')
        temporary.replace(path)
    with path.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    print('DEM_READY',path.name,flush=True)
    return dict(file=path.name,url=item['downloadURL'],title=item['title'],published=item['publicationDate'],metadata=item['metaUrl'],sha256=digest)

def main():
    with urllib.request.urlopen(DISCOVERY,timeout=60) as response:discovery=json.load(response)
    (ROOT/'discovery.json').write_text(json.dumps(discovery,indent=2))
    items=[item for item in discovery['items'] if 'TX_LowerRioGrande_D22' in item['title']]
    assert len(items)==4, 'Review source coverage before changing selected survey'
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(fetch,items))
    n=6144; radius=6371000.; lat,lon=25.9973,-97.1569
    dlat=np.degrees(6000/radius);dlon=dlat/np.cos(np.radians(lat))
    bounds=(lon-dlon,lat-dlat,lon+dlon,lat+dlat)
    transform=from_bounds(*bounds,n,n)
    data=np.full((n,n),np.nan,np.float32)
    for record in records:
        with rasterio.open(ROOT/record['file']) as source:
            patch=np.full((n,n),np.nan,np.float32)
            reproject(rasterio.band(source,1),patch,src_transform=source.transform,src_crs=source.crs,
                src_nodata=source.nodata,dst_transform=transform,dst_crs='EPSG:4326',dst_nodata=np.nan,resampling=Resampling.bilinear,num_threads=2)
            good=np.isfinite(patch)&(patch>-20)&(patch<80);data[good]=patch[good]
            record.update(crs=str(source.crs),source_pixel_m=list(source.res),source_bounds=list(source.bounds),nodata=source.nodata)
    valid=np.isfinite(data)
    assert valid.mean()>.15 and np.all(np.isfinite(data[valid]))
    # Keep validity separate so the renderer can blend across survey coverage.
    np.save(ROOT/'height_m.npy',data);np.save(ROOT/'valid.npy',valid)
    report=dict(source='USGS 3DEP / TX_LowerRioGrande_D22',vertical_datum='NAVD88',pad_offset_m=4.5,
        bounds_wgs84=bounds,output_shape=[n,n],output_pixel_m=12000/n,source_pixel_m=1,coverage=float(valid.mean()),
        range_m=np.percentile(data[valid],[0,50,95,100]).tolist(),records=records)
    (ROOT/'sources.json').write_text(json.dumps(report,indent=2));print('COASTAL_LIDAR_READY',report['coverage'],flush=True)

if __name__=='__main__':main()

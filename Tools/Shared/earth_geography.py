"""Geographic transforms shared by the reproducible terrain and scenery builders."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import math, numpy as np
ROOT=ART_ROOT/'Earth'
R=6371000.;LAT=25.9973;LON=-97.1569
la,lo=map(math.radians,(LAT,LON))
E=np.array([-math.sin(lo),math.cos(lo),0.])
N=np.array([-math.sin(la)*math.cos(lo),-math.sin(la)*math.sin(lo),math.cos(la)])
U=np.array([math.cos(la)*math.cos(lo),math.cos(la)*math.sin(lo),math.sin(la)])
def point(lat,lon,h=0):
    lat,lon=map(math.radians,(lat,lon))
    p=np.array([math.cos(lat)*math.cos(lon),math.cos(lat)*math.sin(lon),math.sin(lat)])*(R+h)
    return float(p@E),float(p@N),float(p@U-R)
def geo(x,y):return LAT+math.degrees(y/R),LON+math.degrees(x/(R*math.cos(la)))
DEM=np.load(ROOT/'Continuity/Elevation/height_m.npy')
VALID=np.load(ROOT/'Continuity/Elevation/valid.npy')
assert DEM.shape==VALID.shape and DEM.ndim==2
assert np.all(np.isfinite(DEM[VALID]))
# Source GeoTIFF elevation is metres NAVD88; ocean/no-data uses sea level.
DEM=np.where(VALID,DEM,0.)
LIDAR_ROOT=ROOT/'LidarCoast'
LIDAR=np.load(LIDAR_ROOT/'height_m.npy',mmap_mode='r') if (LIDAR_ROOT/'height_m.npy').exists() else None
LIDAR_WEIGHT=np.load(LIDAR_ROOT/'blend_weight.npy',mmap_mode='r') if (LIDAR_ROOT/'blend_weight.npy').exists() else None

def sample_field(field,x,y):
    """Pixel-centred, north-up bilinear sampling in the registered 12 km patch."""
    rows,cols=field.shape
    ix=np.clip((np.asarray(x)+6000)/12000*cols-.5,0,cols-1.000001)
    iy=np.clip((6000-np.asarray(y))/12000*rows-.5,0,rows-1.000001)
    i,j=ix.astype(np.int32),iy.astype(np.int32);a,b=ix-i,iy-j
    return (field[j,i]*(1-a)+field[j,i+1]*a)*(1-b)+(field[j+1,i]*(1-a)+field[j+1,i+1]*a)*b

def heights(x,y):
    h=sample_field(DEM,x,y)
    if LIDAR is not None and LIDAR_WEIGHT is not None:
        detail=sample_field(LIDAR,x,y);weight=sample_field(LIDAR_WEIGHT,x,y)
        h=np.where(np.isfinite(detail),h*(1-weight)+np.nan_to_num(detail)*weight,h)
    h=h-4.5
    apron=np.maximum(np.abs(np.asarray(x)-65)-220,np.abs(y)-140)
    road=np.maximum(np.abs(np.asarray(x)-40)-355,np.abs(np.asarray(y)+90)-10)
    blend=np.clip(np.minimum(apron,road)/45,0,1);blend=blend*blend*(3-2*blend)
    return -.35*(1-blend)+h*blend
def height(x,y):
    return float(heights(x,y))

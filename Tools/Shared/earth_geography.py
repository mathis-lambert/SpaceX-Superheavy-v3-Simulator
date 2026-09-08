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
def height(x,y):
    rows,cols=DEM.shape
    ix=max(0,min(cols-1.000001,(x+6000)/12000*cols-.5));iy=max(0,min(rows-1.000001,(6000-y)/12000*rows-.5))
    i,j=int(ix),int(iy);a,b=ix-i,iy-j
    h=float((DEM[j,i]*(1-a)+DEM[j,i+1]*a)*(1-b)+(DEM[j+1,i]*(1-a)+DEM[j+1,i+1]*a)*b)-4.5
    # Preserve the engineered platform and its physical zero-height reference.
    apron=max(abs(x-65)-220,abs(y)-140)
    road=max(abs(x-40)-355,abs(y+90)-10)
    blend=max(0,min(1,min(apron,road)/45))
    blend=blend*blend*(3-2*blend)
    return -.35*(1-blend)+h*blend

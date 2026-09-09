"""Derive continuous coastal coverage and a clearly labelled optical shallows model."""
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
import numpy as np
from scipy.ndimage import distance_transform_edt,gaussian_filter
from PIL import Image

root=ART_ROOT/'Earth';lidar=root/'LidarCoast'
valid=np.load(lidar/'valid.npy');distance=distance_transform_edt(valid)*12000/valid.shape[0]
t=np.clip((distance-4)/32,0,1)
np.save(lidar/'blend_weight.npy',(t*t*(3-2*t)).astype(np.float32))
from earth_geography import heights
axis=(np.arange(4096)+.5)*12000/4096-6000
x,y=np.meshgrid(axis,axis[::-1]);elevation=heights(x,y)+4.5
mask=np.clip((.65-elevation)/.6,0,1);mask=mask*mask*(3-2*mask)
Image.fromarray(np.uint8(mask*255+.5)).save(root/'Continuity/CoastalWaterMask.png')
water=mask>.5;pixel=12000/mask.shape[0]
signed=(distance_transform_edt(water)-distance_transform_edt(~water))*pixel
signed=gaussian_filter(signed,.6)
packed=np.stack([mask,np.clip(.5+signed/256,0,1),np.clip(np.maximum(signed,0)/1200,0,1),np.ones_like(mask)],axis=-1)
Image.fromarray(np.uint8(np.clip(packed*255+.5,0,255))).save(root/'Continuity/CoastalHydrology.png')
report=dict(success=True,pixels=mask.shape[0],extent_m=12000,shore_distance_range_m=[-128,128],
    channels=dict(R='Elevation-derived estimated water coverage',G='Signed shore distance / 256 + 0.5',B='Estimated optical depth factor / 1200 m shore distance',A='Unused'),
    bathymetry='Artist model derived from shore distance, not a bathymetric survey',lidar_boundary_blend_m=32)
(root/'Continuity/coastal-shading.json').write_text(json.dumps(report,indent=2))
print('COASTAL_SHADING_READY',report,flush=True)

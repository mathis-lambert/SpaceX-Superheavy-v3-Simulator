"""Resample landcover onto the DEM to reject offshore no-data elevations."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

from PIL import Image
from pathlib import Path
import numpy as np
root=ART_ROOT/'Earth'
mask=np.zeros((1024,1024),bool)
for j in range(4):
    for i in range(4):
        t=np.asarray(Image.open(root/f'BocaChica_{i}_{j}.png').convert('RGBA').resize((256,256),Image.Resampling.BILINEAR)).astype(float)
        r,g,b,a=t.transpose(2,0,1)
        water=(a<128)|((b>r*1.08)&(g>r*1.04)&(r<150))
        mask[(3-j)*256:(4-j)*256,i*256:(i+1)*256]=water
np.save(root/'water_mask.npy',mask)
print('WATER_COVERAGE',float(mask.mean()))

"""Sample real terrain/landcover for foliage placements; no water or apron grass."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import json, numpy as np
from PIL import Image
from earth_geography import *
from site_landscape import dune_height
tiles={(i,j):np.asarray(Image.open(ROOT/'Registered4000'/f'BocaChica_{i}_{j}.png').convert('RGBA')) for i in (1,2) for j in (1,2)}
rng=np.random.default_rng(2317);grass=[];rocks=[]
for k in range(110000):
    x,y=rng.uniform(-1400,650),rng.uniform(-1400,1400)
    if (-180<x<310 and abs(y)<175) or (abs(y+90)<20 and -350<x<420):continue
    i,j=int((x+6000)/3000),int((y+6000)/3000)
    pixels=tiles[i,j].shape[0]-1
    px=int(((x+6000)/3000-i)*pixels);py=int((1-((y+6000)/3000-j))*pixels)
    r,g,b,a=map(float,tiles[i,j][py,px]);z=dune_height(x,y)
    if a<200 or z<-4.1:continue
    if g>r*1.025 and g>b*1.06 and 35<g<155:
        pos=point(*geo(x,y),z-.04);grass.append([*pos,float(rng.uniform(0,360)),float(rng.uniform(.65,1.55))])
    elif r>110 and g>100 and b<r*.91 and rng.random()<.008:
        rocks.append([*point(*geo(x,y),z-.12),float(rng.uniform(0,360)),float(rng.uniform(.25,.65))])
(ROOT/'scenery.json').write_text(json.dumps(dict(grass=grass,rocks=rocks)))
print('SCENERY',len(grass),len(rocks))

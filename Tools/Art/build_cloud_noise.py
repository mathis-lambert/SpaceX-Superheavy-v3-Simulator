"""Deterministic tileable coherent/Worley cloud fields; no external asset licence.

R: broad coherent billows, G: cellular erosion, B: fine erosion. The weather
map is evaluated on a sphere, so both longitude seam and poles are continuous.
"""
from pathlib import Path
import sys, json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
import numpy as np
from PIL import Image

out=ART_ROOT/'Weather';out.mkdir(parents=True,exist_ok=True)
rng=np.random.default_rng(902610)
size=128
z,y,x=np.mgrid[:size,:size,:size].astype(np.float32)/size
coords=np.array([z,y,x])
def smooth_noise(c,frequency):
    grid=rng.random((frequency,)*3).astype(np.float32)
    q=c*frequency;i=np.floor(q).astype(np.int32);t=q-i
    t=t*t*t*(t*(t*6-15)+10)
    result=np.zeros(c.shape[1:],dtype=np.float32)
    for a in (0,1):
        for b in (0,1):
            for d in (0,1):
                weight=(t[0] if a else 1-t[0])*(t[1] if b else 1-t[1])*(t[2] if d else 1-t[2])
                result+=grid[(i[0]+a)%frequency,(i[1]+b)%frequency,(i[2]+d)%frequency]*weight
    return result
def worley(frequency):
    points=rng.random((frequency,frequency,frequency,3)).astype(np.float32)
    q=np.moveaxis(coords*frequency,0,-1);cell=np.floor(q).astype(np.int32)
    distance=np.full(q.shape[:-1],10,dtype=np.float32)
    for a in (-1,0,1):
        for b in (-1,0,1):
            for c in (-1,0,1):
                neighbour=cell+np.array([a,b,c],dtype=np.int32)
                jitter=points[neighbour[...,0]%frequency,neighbour[...,1]%frequency,neighbour[...,2]%frequency]
                d=q-neighbour-jitter
                distance=np.minimum(distance,np.sum(d*d,axis=-1))
    return np.clip(1-np.sqrt(distance),0,1)
perlin=sum(smooth_noise(coords,f)*w for f,w in [(4,.55),(8,.3),(16,.15)])
w4,w8,w16=(worley(f) for f in (4,8,16))
base=np.clip((perlin*.7+w4*.3-.16)/.64,0,1)
volume=np.stack([base,w8,w16,np.ones_like(base)],axis=-1)
atlas=np.zeros((1024,2048,4),np.uint8)
for i in range(size):atlas[(i//16)*size:(i//16+1)*size,(i%16)*size:(i%16+1)*size]=(volume[i]*255).astype(np.uint8)
Image.fromarray(atlas).save(out/'CloudShape128.png')
lat,lon=np.mgrid[:1024,:2048].astype(np.float32)
lat=(lat+.5)/1024*np.pi;lon=(lon+.5)/2048*2*np.pi
sphere=np.array([np.cos(lat),np.sin(lat)*np.sin(lon),np.sin(lat)*np.cos(lon)])*.5+.5
weather=sum(smooth_noise(sphere,f)*w for f,w in [(36,.4),(90,.35),(192,.25)])
weather=np.clip((weather-.25)/.5,0,1)
types=np.clip(smooth_noise(sphere,32),0,1)
high=np.clip(smooth_noise(sphere*np.array([1.,.6,1.4])[:,None,None],70)*.6+smooth_noise(sphere,180)*.4,0,1)
Image.fromarray((np.stack([weather,types,high],axis=-1)*255).astype(np.uint8)).save(out/'CloudWeather2048.png')
(out/'manifest.json').write_text(json.dumps(dict(seed=902610,volume_resolution=128,weather_resolution=[2048,1024],channels=['Coherent-Worley shape','Worley erosion','fine Worley erosion'],license='Project-authored procedural textures'),indent=2))
print('CLOUD_NOISE_READY',out)

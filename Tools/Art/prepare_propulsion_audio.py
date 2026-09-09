"""Original deterministic 48 kHz layers for propulsion and ground operations.

The existing NASA STS-131 roar remains separately credited in Audio/CREDITS.json.
These sounds are authored approximations, not Raptor or tower field recordings.
"""
import sys, json, hashlib, wave
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import PROJECT_ROOT, ART_ROOT
sys.path.insert(0,str(PROJECT_ROOT/'Saved/ToolDependencies'))
import numpy as np

RATE=48000
OUT=ART_ROOT/'Audio';OUT.mkdir(parents=True,exist_ok=True)
rng=np.random.default_rng(20260909)
report={}

def noise(n,low,high,slope=0):
    f=np.fft.rfftfreq(n,1/RATE)
    shape=(f/np.maximum(f+low,1))**3*np.exp(-(f/high)**2)/np.maximum(f,20)**slope
    a=np.fft.irfft(np.fft.rfft(rng.standard_normal(n))*shape,n)
    return a/max(np.std(a),1.e-9)

def save(name,a,loop=True):
    a-=np.mean(a)
    a=np.tanh(a*.35)
    if not loop:
        a*=np.minimum(1,np.arange(len(a))/(RATE*.003))*np.minimum(1,np.arange(len(a))[::-1]/(RATE*.05))
    else:
        # A short equal-power overlap removes the seam without periodic silence.
        k=RATE//3;t=np.arange(k)/k
        join=a[-k:]*np.cos(t*np.pi/2)+a[:k]*np.sin(t*np.pi/2)
        a=np.concatenate((join,a[k:-k]))
    a*=.72/max(np.max(np.abs(a)),1.e-9)
    path=OUT/(name+'.wav')
    with wave.open(str(path),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes((a*32767).astype('<i2').tobytes())
    with wave.open(str(path),'rb') as w:
        sr=w.getframerate();audio=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2')/32768.
    assert sr==RATE and np.isfinite(audio).all() and np.max(np.abs(audio))<.73
    report[name]={'seconds':len(audio)/RATE,'peak':float(np.max(np.abs(audio))),
        'rms':float(np.sqrt(np.mean(audio**2))),'loop':loop,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
        'seam_delta':float(abs(audio[0]-audio[-1]))}

n=RATE*13;t=np.arange(n)/RATE
rumble=noise(n,18,240,.7)*(.8+.13*np.sin(2*np.pi*t*7/13)+.07*np.sin(2*np.pi*t*19/13))
save('EngineRumble',rumble)
crackle=noise(n,220,9500,.12)
mod=noise(n,2,24,0);mod=np.maximum(mod-.4,0)
save('EngineCrackle',crackle*(.2+mod*.8))
save('CryogenicHiss',noise(n,450,13000,.2)*(.8+.12*np.sin(2*np.pi*t*4/13)))
save('Deluge',noise(n,80,4700,.55)*(.85+.15*np.sin(2*np.pi*t*9/13)))
drive=.25*noise(n,120,3500,.25)
for hz,gain in [(83,.22),(167,.12),(251,.08),(499,.035)]:drive+=gain*np.sin(2*np.pi*hz*t+.05*np.sin(2*np.pi*t*11/13))
save('TowerDrive',drive)
n=RATE*3;t=np.arange(n)/RATE
contact=noise(n,80,6500,.1)*np.exp(-t*26)*1.8
for hz,gain,decay in [(74,.6,1.8),(147,.28,2.6),(239,.20,3.2),(418,.13,4),(719,.075,5.2)]:
    contact+=gain*np.sin(2*np.pi*hz*t)*np.exp(-t*decay)
save('TowerContact',contact,False)
release=noise(n,260,9000,.2)*(np.exp(-t*12)+.3*np.exp(-np.maximum(0,t-.16)*9)*(t>.16))
release+=.32*np.sin(2*np.pi*181*t)*np.exp(-t*7)
save('MountRelease',release,False)
(OUT/'PROPULSION_CREDITS.json').write_text(json.dumps({'author':'Original simulator sound design',
    'method':'Deterministic filtered noise, amplitude modulation and damped inharmonic resonances.',
    'sample_rate':RATE,'assets':report},indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))

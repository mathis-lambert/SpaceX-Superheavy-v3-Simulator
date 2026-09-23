"""Check an actual Unreal master-submix recording for silence and clipping."""
import argparse, array, hashlib, json, math, sys, wave
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture',type=Path)
args=parser.parse_args()
with wave.open(str(args.capture),'rb') as stream:
    assert stream.getsampwidth()==2,'Expected Unreal PCM16 recording'
    rate=stream.getframerate();channels=stream.getnchannels()
    samples=array.array('h',stream.readframes(stream.getnframes()))
if sys.byteorder!='little':samples.byteswap()
peak=max((abs(s) for s in samples),default=0)/32768
rms=math.sqrt(sum((s/32768)**2 for s in samples)/max(1,len(samples)))
clipped=sum(abs(s)>=32760 for s in samples)
result={'success':peak>.005 and rms>.0001 and clipped==0,
    'source':'Actual Unreal master-submix capture','sample_rate':rate,'channels':channels,
    'duration_s':len(samples)/max(1,rate*channels),'peak':peak,'rms':rms,'clipped_samples':clipped,
    'sha256':hashlib.sha256(args.capture.read_bytes()).hexdigest()}
args.capture.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
if not result['success']:raise SystemExit('Recorded mix is silent or clipped')

"""Summarize comparable Unreal CSV captures; exclude startup and metadata rows."""
import sys
from pathlib import Path
import csv,json,statistics

path=Path(sys.argv[1]);all_rows=list(csv.DictReader(path.open(encoding='utf-8-sig')))
rows=all_rows[300:-2]
assert rows, 'Capture is shorter than the warm-up exclusion'
stats={}
for key in rows[0]:
    if key is None:continue
    if key in ['FrameTime','GameThreadTime','RenderThreadTime','GPUTime'] or key.startswith('GPU/'):
        try:
            values=[float(r[key]) for r in rows]
            stats[key]={'mean_ms':round(statistics.mean(values),3),'p95_ms':round(sorted(values)[int(len(values)*.95)],3)}
        except (ValueError,TypeError):pass
result={'capture':str(path),'frames_measured':len(rows),'fps_from_mean':round(1000/stats['FrameTime']['mean_ms'],2),'timings':dict(sorted(stats.items(),key=lambda x:-x[1]['mean_ms']))}
path.with_suffix('.summary.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps({**result,'timings':dict(list(result['timings'].items())[:12])},indent=2))

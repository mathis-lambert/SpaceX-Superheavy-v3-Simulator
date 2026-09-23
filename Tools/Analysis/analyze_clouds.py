"""Summarize raw UE GPU CSVs; optional explicit cloud-budget regression gate."""
import argparse,csv,json,statistics
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('captures',nargs='+',type=Path)
p.add_argument('--output',required=True,type=Path);p.add_argument('--cloud-budget-ms',type=float)
a=p.parse_args();results=[]
for path in a.captures:
    rows=[]
    for row in csv.DictReader(path.open(encoding='utf-8-sig')):
        try:
            if float(row['FrameTime'])>0:rows.append(row)
        except (ValueError,TypeError,KeyError):continue
    if len(rows)<300:raise RuntimeError(f'Incomplete capture: {path}')
    def summary(key):
        values=sorted(float(row[key]) for row in rows)
        return dict(mean_ms=statistics.mean(values),p95_ms=values[int(len(values)*.95)],max_ms=max(values))
    frame=summary('FrameTime');cloud=summary('GPU/VolumetricCloud')
    if cloud['mean_ms']<=0:raise RuntimeError(f'Cloud GPU timing missing: {path}')
    result=dict(capture=path.name,frames=len(rows),frame=frame,cloud=cloud,mean_fps=1000/frame['mean_ms'])
    if a.cloud_budget_ms is not None:result['budget_pass']=cloud['p95_ms']<=a.cloud_budget_ms
    results.append(result)
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text(json.dumps(results,indent=2))
for r in results:print(f"{r['capture']}: {r['mean_fps']:.1f} FPS / cloud {r['cloud']['mean_ms']:.2f} ms / p95 {r['cloud']['p95_ms']:.2f} ms")
if any(not r.get('budget_pass',True) for r in results):raise SystemExit('Cloud GPU budget exceeded')

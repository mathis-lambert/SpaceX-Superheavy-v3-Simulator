"""Check imagery registration, shared edges and the site's east/north convention."""
import sys,json,hashlib,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from earth_geography import point,geo,height
from site_landscape import dune_height,ROAD

folder=ART_ROOT/'Earth/Registered4000'
tiles=json.loads((folder/'sources.json').read_text())['tiles'];assert len(tiles)==16
by_name={r['file']:r for r in tiles};error=0
for r in tiles:
    assert hashlib.sha256((folder/r['file']).read_bytes()).hexdigest()==r['sha256']
    assert r['dimensions']==[4000,4000]
    error=max(error,max(abs(a-b) for a,b in zip(r['bbox'],r['reported_extent'])))
    assert error<1e-8
for j in range(4):
    for i in range(4):
        bounds=by_name[f'BocaChica_{i}_{j}.png']['reported_extent']
        if i<3:assert abs(bounds[2]-by_name[f'BocaChica_{i+1}_{j}.png']['reported_extent'][0])<1e-8
        if j<3:assert abs(bounds[3]-by_name[f'BocaChica_{i}_{j+1}.png']['reported_extent'][1])<1e-8
east=point(*geo(1000,0));north=point(*geo(0,1000))
assert east[0]>999 and abs(east[1])<.1
assert north[1]>999 and abs(north[0])<.1
for x,y in [(0,0),(24,0),(-120,0),(65,130),(600,3000)]:
    assert abs(dune_height(x,y)-height(x,y))<1e-6,(x,y)
assert ROAD[0]==ROAD[-1]
assert all(math.dist(a,b)>1 for a,b in zip(ROAD,ROAD[1:]))
report=dict(success=True,registered_tiles=len(tiles),max_extent_error_degrees=error,shared_boundaries=24,world_axes='X east, Y true north, Z up',road_length_m=sum(math.dist(a,b) for a,b in zip(ROAD,ROAD[1:])))
SAVED_ROOT.mkdir(parents=True,exist_ok=True)
(SAVED_ROOT/'site-geography-check.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))

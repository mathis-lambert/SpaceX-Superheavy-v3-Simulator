"""Collect the current world/flight wave's measured reports without changing assets."""
from pathlib import Path
import json,hashlib,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import PROJECT_ROOT,SAVED_ROOT

def read(name):return json.loads((SAVED_ROOT/name).read_text(encoding='utf-8-sig'))
matrix=read('physical-matrix.json');assert len(matrix)==9 and all(x['success'] for x in matrix)
flights=[read(Path(x['report']).name) for x in matrix]
units=read('WorldUnitTests/index.json');assert units['succeeded']==4 and units['failed']==0
render=read('experience-flight-audit.json');assert render['success'] and render['chase_contact_frames']>30
world=read('WorldAudit/result.json');assert world['success']
interface=read('InterfaceAudit/result.json');assert interface['success']
contacts=[read('Contact'+n+'.json') for n in ('Centered','WrongHeading','SideImpact')];assert all(x['success'] for x in contacts)
assets=read('experience-asset-audit.json');assert assets['success']
bench={}
for scene in ('Home','Launch'):
    for mode in ('Native','DLSSQuality'):
        name=f'WorldContinuity{scene}{mode}1440.summary.json';bench[scene+mode]=read(name)
result=dict(success=True,flight_runs=len(flights),units_passed=units['succeeded'],contact_fixtures=len(contacts),
    peak_altitude_m=[min(x['peak_altitude_m'] for x in flights),max(x['peak_altitude_m'] for x in flights)],
    max_support_drift_m=max(x['restraint_drift_m'] for x in flights),max_capture_error_m=max(x['capture_error_m'] for x in flights),
    landing_burn_s=[min(x['landing_burn_seconds'] for x in flights),max(x['landing_burn_seconds'] for x in flights)],
    max_propellant_balance_error_kg=max(abs(x['propellant_balance_error_kg']) for x in flights),
    blueprints=len(assets['blueprints']),dependencies=len(assets['dependencies']),
    render=render,world_checks=world['checks'],interface_checks=interface['checks'],benchmarks=bench)
sources=sorted((PROJECT_ROOT/'Source').rglob('*.cpp'))+sorted((PROJECT_ROOT/'Source').rglob('*.h'))
result['source_sha256']={str(p.relative_to(PROJECT_ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
(SAVED_ROOT/'world-validation-summary.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('source_sha256','benchmarks','interface_checks','world_checks','render')},indent=2))

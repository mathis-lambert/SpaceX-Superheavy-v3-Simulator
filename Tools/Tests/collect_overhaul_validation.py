"""Aggregate the current overhaul reports; historical validations stay separate."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import json,hashlib
from pathlib import Path
root=PROJECT_ROOT;saved=root/'Saved'/'Recovery'
def read(name):return json.loads((saved/name).read_text(encoding='utf-8-sig'))
matrix=read('physical-matrix.json');assert len(matrix)==9 and all(r['success'] for r in matrix)
out={'success':True,'physical_matrix':[],'contact_fixtures':{},'audits':{},'rendered_flight':{},'sources':{}}
for case in matrix:
    r=read(Path(case['report']).name)
    assert r['success'] and r['physical_capture'] and r['left_rail_contact'] and r['right_rail_contact'] and r['contact_engine_shutdown']
    assert not r['unpowered_thrust_violation'] and r['launch_catch_axis_distance_m']<.01
    assert 80000<r['peak_altitude_m']<130000 and r['boostback_ignition_altitude_m']>50000
    assert r['unpowered_seconds']>60 and r['fin_control_seconds']>5
    assert r['capture_lug_error_m']<.35 and r['capture_heading_error_deg']<2 and r['final_speed_mps']<.1
    assert r['propellant_remaining_kg']>0 and abs(r['propellant_remaining_kg']+r['main_propellant_consumed_kg']-3650000)<.1
    out['physical_matrix'].append(dict(case,peak_altitude_m=r['peak_altitude_m'],capture_speed_mps=r['capture_speed_mps'],support_drift_m=r['restraint_drift_m'],mass_kg=r['mass_kg']))
for name in ['Centered','WrongHeading','SideImpact']:
    r=read('Contact'+name+'.json');assert r['success'];out['contact_fixtures'][name]={'success':True,'duration_s':r['duration_s'],'report':'Saved/Recovery/Contact'+name+'.json'}
for name,path in [('presentation','Overhaul/result.json'),('assets','asset-audit.json'),('earth','earth-asset-audit.json'),('vfx','vfx-asset-audit.json'),('camera','EarthAudit/result.json')]:
    r=read(path);assert r['success'],name;out['audits'][name]={'success':True,'report':'Saved/Recovery/'+path}
r=read('OverhaulRenderedFlight.json');assert r['success'] and r['left_rail_contact'] and r['right_rail_contact'] and r['contact_engine_shutdown']
out['rendered_flight']=r
r=read('OverhaulRenderedFinal15.json');assert r['success'] and r['left_rail_contact'] and r['right_rail_contact'] and r['contact_engine_shutdown']
out['rendered_final15']=r
for path in [root/'Source/SuperHeavySim/Private/Recovery/SuperHeavyRecoveryDirector.cpp',root/'Source/SuperHeavySim/Private/Recovery/RecoveryContacts.cpp',root/'Source/SuperHeavySim/Private/Recovery/RecoverySkyComponent.cpp',ART_ROOT/'VehicleDetails/sources.json']:
    out['sources'][str(path.relative_to(root.parent))]=hashlib.sha256(path.read_bytes()).hexdigest()
out['limitations']=['Estimated flight and aerodynamic model, not SpaceX telemetry.','Rigid contact volumes, without arm deformation or detailed bearing contact.','Authored gas nozzles and Starship; visual upper-stage trajectory after separation.','Art-directed plume and atlas smoke, not real-time CFD.','Historical NASA/NAIP imagery; no real-time site survey or guaranteed optical resolution.']
target=root/'Docs'/'OVERHAUL_VALIDATION.json';target.write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf-8')
print('OVERHAUL_VALIDATION_PASS',len(matrix),'physical flights, 3 contact fixtures, rendered flight and asset/UI audits')

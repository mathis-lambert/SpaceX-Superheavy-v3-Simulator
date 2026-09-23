"""Install the moving-rail artwork and resampled vegetation without duplicating surfaces."""
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from unreal_imports import import_scenery_mesh
from unreal_materials import u,A,prop
root='/Game/Starbase'
path=root+'/Meshes/SM_CaptureArm'
previous=u.load_asset(path);assert previous
materials=[previous.get_material(i) for i in range(len(previous.static_materials))]
mesh=import_scenery_mesh(ART_ROOT/'Flight/SM_CaptureArm.fbx',path)
for i,mat in enumerate(materials[:len(mesh.static_materials)]):mesh.set_material(i,mat)
nanite=mesh.get_editor_property('nanite_settings');prop(nanite,'enabled',True);prop(mesh,'nanite_settings',nanite)
assert A.save_loaded_asset(mesh,False)
profile=u.load_asset(root+'/Data/DA_RecoveryEnvironment');assert profile
scenery=json.loads((ART_ROOT/'Earth/scenery.json').read_text())
for field in ('grass','rocks'):
    prop(profile,field,[u.Transform(location=u.Vector(x*100,y*100,z*100),rotation=u.Rotator(yaw=yaw),scale=u.Vector(s,s,s)) for x,y,z,yaw,s in scenery[field]])
assert A.save_loaded_asset(profile,False)
(SAVED_ROOT/'coast-mechanics-finalized.json').write_text(json.dumps(dict(success=True,grass=len(scenery['grass']),rocks=len(scenery['rocks']),moving_rail_separate=True),indent=2))
print('COAST_MECHANICS_FINALIZED',flush=True)

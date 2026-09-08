"""Import source-derived relief without replacing level actors or flight collision."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from unreal_materials import u,A,prop
from unreal_imports import import_scenery_mesh
import json

root='/Game/Starbase';source=ART_ROOT/'Earth/Continuity/Meshes'
report=[]
for path in sorted(source.glob('SM_*.fbx')):
    name=path.stem;asset_path=root+'/Meshes/Earth/'+name
    mesh=import_scenery_mesh(path,asset_path)
    mesh.set_material(0,u.load_asset(root+'/Materials/Earth/'+name.replace('SM_','M_',1)))
    settings=mesh.get_editor_property('nanite_settings');prop(settings,'enabled',name.startswith('SM_BocaChica'))
    if name.startswith('SM_BocaChica'):prop(settings,'position_precision',2)
    prop(mesh,'nanite_settings',settings);assert A.save_loaded_asset(mesh,False)
    report.append(name)
assert len(report)==19
profile=u.load_asset(root+'/Data/DA_RecoveryEnvironment');scenery=json.loads((ART_ROOT/'Earth/scenery.json').read_text())
for field in ('grass','rocks'):
    transforms=[u.Transform(location=u.Vector(x*100,y*100,z*100),rotation=u.Rotator(yaw=yaw),scale=u.Vector(s,s,s)) for x,y,z,yaw,s in scenery[field]]
    prop(profile,field,transforms)
assert A.save_loaded_asset(profile,False)
(SAVED_ROOT/'terrain-relief-assets.json').write_text(json.dumps(dict(success=True,meshes=report,source='USGS 3DEP 1/3 arcsecond GeoTIFF',local_grid_spacing_m=3000/256,globe_segments=[1024,512]),indent=2))
print('TERRAIN_RELIEF_READY')

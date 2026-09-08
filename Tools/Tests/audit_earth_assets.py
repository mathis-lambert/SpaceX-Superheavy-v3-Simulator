"""Read-only contract check of the authored world; render checks remain separate."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u,json
from pathlib import Path
root='/Game/Starbase';levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level(root+'/Maps/L_RecoveryLab')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
earth={a.get_actor_label():a for a in actors if a.get_actor_label().startswith('Recovery_Earth_SM_')}
assert len(earth)==19,len(earth)
assert not any(a.get_actor_label() in ['Recovery_FlightOcean','Recovery_FlightCoast'] for a in actors)
out={'success':True,'surfaces':len(earth),'local_nanite_tiles':0,'textures':19}
for name,a in earth.items():
    c=a.static_mesh_component;mesh=c.get_editor_property('static_mesh')
    assert c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION,(name,str(c.get_collision_enabled()))
    assert not c.get_editor_property('cast_shadow')
    nanite=mesh.get_editor_property('nanite_settings').get_editor_property('enabled')
    if 'BocaChica' in name:assert nanite;out['local_nanite_tiles']+=1
    else:assert not nanite
    assert mesh.get_material(0)
    assert a.get_actor_scale3d()==u.Vector(1,-1,1)
assert out['local_nanite_tiles']==16
atmos=[a for a in actors if isinstance(a,u.SkyAtmosphere)];assert len(atmos)==1
c=atmos[0].get_component_by_class(u.SkyAtmosphereComponent)
assert abs(atmos[0].get_actor_location().z+637100000)<1
assert c.get_editor_property('bottom_radius')==6370.5
assert c.get_editor_property('atmosphere_height')==100.5
sun=next(a for a in actors if a.get_actor_label()=='Recovery_Sun')
assert sun.get_actor_rotation().pitch<0
assert sun.light_component.get_editor_property('per_pixel_atmosphere_transmittance')
t=u.load_asset(root+'/Textures/Earth/T_EarthSeptember')
assert t.get_editor_property('max_texture_size')==16384
assert not t.get_editor_property('virtual_texture_streaming')
profile=u.load_asset(root+'/Data/DA_RecoveryEnvironment')
out['grass']=len(profile.get_editor_property('grass'));out['rocks']=len(profile.get_editor_property('rocks'))
assert out['grass']>8000 and out['rocks']>100
path=Path(u.Paths.project_saved_dir())/'Recovery'/'earth-asset-audit.json'
path.write_text(json.dumps(out,indent=2));print('EARTH_ASSET_AUDIT',out)

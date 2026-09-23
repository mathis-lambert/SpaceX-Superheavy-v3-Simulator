"""Install registered terrain and continuous service routes."""
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from unreal_materials import u,prop
from unreal_imports import import_scenery_mesh
from surface_materials import road_surface
ROOT='/Game/Starbase'
SRC=ART_ROOT/'Starbase/Landscape'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);assert levels.load_level(ROOT+'/Maps/L_RecoveryLab')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)

road=road_surface('M_ServiceRoad');shoulder=road_surface('M_ServiceShoulder',True)
# Replace source meshes at their existing asset paths; no additional terrain layer.
for j in (() if (ART_ROOT/'Earth/LidarCoast/Meshes/geometry.json').exists() else (1,2)):
    for i in (1,2):
        name=f'SM_BocaChica_{i}_{j}'
        mesh=import_scenery_mesh(SRC/(name+'.fbx'),ROOT+'/Meshes/Earth/'+name)
        mesh.set_material(0,u.load_asset(ROOT+f'/Materials/Earth/M_BocaChica_{i}_{j}'))
        nanite=mesh.get_editor_property('nanite_settings');prop(nanite,'enabled',True);prop(nanite,'position_precision',2);prop(mesh,'nanite_settings',nanite)
        u.EditorAssetLibrary.save_loaded_asset(mesh,False)

names={'SM_ServiceRoadNetwork':road,'SM_ServiceRoadShoulder':shoulder}
for actor in actors.get_all_level_actors():
    label=actor.get_actor_label()
    if label in ('Recovery_ServiceRoad',) or label.startswith('Recovery_RoadDash') or label.startswith('Recovery_Landscape_'):
        actors.destroy_actor(actor)
for name,mat in names.items():
    mesh=import_scenery_mesh(SRC/(name+'.fbx'),ROOT+'/Meshes/Starbase/'+name);mesh.set_material(0,mat)
    u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    actor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector());actor.set_actor_label('Recovery_Landscape_'+name)
    actor.set_folder_path('Recovery/Environment/Landscape');actor.set_actor_scale3d(u.Vector(1,-1,1))
    c=actor.static_mesh_component;c.set_static_mesh(mesh);c.set_collision_profile_name('NoCollision',False);c.set_cast_shadow(False)
    prop(c,'affect_distance_field_lighting',False);prop(c,'visible_in_ray_tracing',False)

profile=u.load_asset(ROOT+'/Data/DA_RecoveryEnvironment')
scenery=json.loads((ART_ROOT/'Earth/scenery.json').read_text())
for field in ('grass','rocks'):
    prop(profile,field,[u.Transform(location=u.Vector(x*100,y*100,z*100),rotation=u.Rotator(yaw=yaw),scale=u.Vector(s,s,s)) for x,y,z,yaw,s in scenery[field]])
layout=json.loads((SRC/'geometry.json').read_text())
prop(profile,'service_road',[u.Vector(x,y,.07) for x,y in layout['road']])
u.EditorAssetLibrary.save_loaded_asset(profile,False);levels.save_current_level()
(SAVED_ROOT/'site-landscape-assets.json').write_text(json.dumps(dict(success=True,meshes=layout['meshes'],grass=len(scenery['grass']),rocks=len(scenery['rocks'])),indent=2))
print('SITE_LANDSCAPE_ASSETS_READY')

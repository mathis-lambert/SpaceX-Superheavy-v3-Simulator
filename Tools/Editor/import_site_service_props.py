"""Import reproducible service equipment, then place it outside flight hardware."""
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
from unreal_materials import u,A,E,prop,material,constant,color,connect,save
from unreal_imports import import_scenery_mesh
root='/Game/Starbase';mats={}
for name,col,metal,rough in [('Paint',(.63,.67,.7),.35,.32),('Rubber',(.014,.017,.021),0,.84),('Glass',(.035,.065,.08),.25,.10),('Metal',(.24,.27,.29),.9,.29),('Lamp',(.95,.74,.3),0,.25)]:
    m=material(root+'/Materials/Starbase/M_Service'+name);connect(m,color(m,col),u.MaterialProperty.MP_BASE_COLOR)
    connect(m,constant(m,metal),u.MaterialProperty.MP_METALLIC);connect(m,constant(m,rough),u.MaterialProperty.MP_ROUGHNESS)
    if name=='Lamp':connect(m,color(m,(1.5,.8,.18)),u.MaterialProperty.MP_EMISSIVE_COLOR)
    save(m);mats[name]=m
meshes={}
for name in ('SM_ServicePickup','SM_TowableGenerator'):
    mesh=import_scenery_mesh(ART_ROOT/'Starbase/Service'/(name+'.fbx'),root+'/Meshes/Starbase/'+name)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        label=str(slot.get_editor_property('imported_material_slot_name')).split('.')[0]
        assert label in mats,label;mesh.set_material(i,mats[label])
    settings=mesh.get_editor_property('nanite_settings');prop(settings,'enabled',True);prop(mesh,'nanite_settings',settings)
    assert A.save_loaded_asset(mesh,False);meshes[name]=mesh
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);assert levels.load_level(root+'/Maps/L_RecoveryLab')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith('Recovery_Service_'):actors.destroy_actor(actor)
placements=[]
for i in range(4):placements.append(('SM_ServicePickup',(-102+i*7,-113,0),90 if i%2 else -90))
for i in range(3):placements.append(('SM_TowableGenerator',(-134,85+i*9,0),0))
for i,(name,pos,yaw) in enumerate(placements):
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*[x*100 for x in pos]),u.Rotator(yaw=yaw));a.set_actor_label(f'Recovery_Service_{name}_{i}');a.set_folder_path('Recovery/Starbase/Service')
    c=a.static_mesh_component;c.set_static_mesh(meshes[name]);c.set_collision_profile_name('NoCollision',False);c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    c.set_cast_shadow(True);prop(c,'ld_max_draw_distance',180000.);a.set_actor_scale3d(u.Vector(1,-1,1))
levels.save_current_level()
(SAVED_ROOT/'site-service-assets.json').write_text(json.dumps(dict(success=True,models=list(meshes),actors=len(placements),source='Project-authored generic equipment, not surveyed assets',collision='Scenery only; no flight contact'),indent=2))
print('SITE_SERVICE_ASSETS_READY')

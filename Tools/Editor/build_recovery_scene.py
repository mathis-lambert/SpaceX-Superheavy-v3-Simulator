"""Reproducible authoring of the Recovery Lab. Run in Unreal's PythonScript commandlet."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u, os, math

ROOT='/Game/Starbase'
LEVEL=ROOT+'/Maps/L_RecoveryLab'
assets=u.AssetToolsHelpers.get_asset_tools()
library=u.EditorAssetLibrary
editor=u.get_editor_subsystem(u.EditorActorSubsystem)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)

def material(name,color,metal=0,rough=0.5,emission=0):
    path=ROOT+'/Materials/'+name
    if library.does_asset_exist(path): return u.load_asset(path)
    m=assets.create_asset(name,ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
    e=u.MaterialEditingLibrary
    c=e.create_material_expression(m,u.MaterialExpressionConstant3Vector,0,0)
    c.set_editor_property('constant',u.LinearColor(*color,1))
    e.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(metal,u.MaterialProperty.MP_METALLIC,150),(rough,u.MaterialProperty.MP_ROUGHNESS,250)]:
        n=e.create_material_expression(m,u.MaterialExpressionConstant,0,y);n.set_editor_property('r',value);e.connect_material_property(n,'',prop)
    if emission:
        glow=e.create_material_expression(m,u.MaterialExpressionConstant3Vector,0,350)
        glow.set_editor_property('constant',u.LinearColor(*(v*emission for v in color),1));e.connect_material_property(glow,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    e.recompile_material(m);library.save_loaded_asset(m);return m

steel=material('M_Graphite',(0.045,0.075,0.095),0.8,0.35)
light=material('M_Cladding',(0.42,0.5,0.55),0.5,0.42)
amber=material('M_SafetyAmber',(0.95,0.24,0.025),0.3,0.4)
concrete=material('M_Concrete',(0.16,0.18,0.18),0,0.88)
for instanced in (steel,light,amber,concrete):
    instanced.set_editor_property('used_with_instanced_static_meshes',True)
    u.MaterialEditingLibrary.recompile_material(instanced);library.save_loaded_asset(instanced)
asphalt=material('M_Asphalt',(0.023,0.031,0.039),0,0.88)
white=material('M_Marking',(0.8,0.82,0.75),0,0.7)
water=material('M_OceanRipples',(0.008,0.038,0.052),0.2,0.28)
green=material('M_CoastalLand',(0.032,0.055,0.045),0,0.97)
glow=material('M_GuidanceLight',(0.08,0.8,0.59),0,0.4,12)
joint=material('M_ExpansionJoint',(0.045,0.055,0.058),0,0.95)

# Analytical world-space ripple normals, animated without external texture dependencies.
if not library.get_metadata_tag(water,'RecoveryRipples'):
    e=u.MaterialEditingLibrary
    position=e.create_material_expression(water,u.MaterialExpressionWorldPosition,-500,500)
    time=e.create_material_expression(water,u.MaterialExpressionTime,-500,700)
    ripple=e.create_material_expression(water,u.MaterialExpressionCustom,-200,500)
    ripple.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    a=u.CustomInput();a.set_editor_property('input_name','P')
    b=u.CustomInput();b.set_editor_property('input_name','T')
    ripple.set_editor_property('inputs',[a,b])
    ripple.set_editor_property('code','float2 q=P.xy*0.003; return normalize(float3(0.025*sin(q.x+q.y*0.38+T*0.7)+0.01*sin(q.x*2.3-q.y*0.8-T),0.022*cos(q.y+q.x*0.44+T*0.6)+0.012*sin(q.y*1.9-q.x*0.31+T*0.8),1));')
    e.connect_material_expressions(position,'',ripple,'P');e.connect_material_expressions(time,'',ripple,'T')
    e.connect_material_property(ripple,'',u.MaterialProperty.MP_NORMAL)
    e.recompile_material(water);library.set_metadata_tag(water,'RecoveryRipples','1');library.save_loaded_asset(water)

src=str(ART_ROOT/'Flight')
for name in ['SM_TowerServiceCore','SM_CaptureArm','SM_CoastalTerrain']:
    if not library.does_asset_exist(ROOT+'/Meshes/'+name):
        task=u.AssetImportTask();task.filename=os.path.join(src,name+'.fbx');task.destination_path=ROOT+'/Meshes';task.destination_name=name
        task.automated=True;task.save=True;task.replace_existing=True
        opts=u.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
        opts.set_editor_property('automated_import_should_detect_type',False)
        opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
        opts.static_mesh_import_data.combine_meshes=True
        task.options=opts;assets.import_asset_tasks([task])
    mesh=u.load_asset(ROOT+'/Meshes/'+name)
    if mesh:
        for i in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(i,[steel,light,amber][i%3])
        library.save_loaded_asset(mesh)

def blueprint(name,parent):
    path=ROOT+'/Blueprints/'+name
    if library.does_asset_exist(path):return u.load_asset(path)
    f=u.BlueprintFactory();f.set_editor_property('parent_class',u.load_class(None,'/Script/SuperHeavySim.'+parent))
    bp=assets.create_asset(name,ROOT+'/Blueprints',u.Blueprint,f)
    u.BlueprintEditorLibrary.compile_blueprint(bp);library.save_loaded_asset(bp);return bp

tower_bp=blueprint('BP_LaunchTower','SuperHeavyLaunchTower')
director_bp=blueprint('BP_RecoveryDirector','SuperHeavyRecoveryDirector')
# Store artwork on the reusable Blueprint as well as the authored site instance.
tower_defaults=u.get_default_object(tower_bp.generated_class());tower_defaults.modify()
tower_defaults.get_editor_property('structure').set_material(0,steel)
tower_defaults.get_editor_property('architectural_details').set_static_mesh(u.load_asset(ROOT+'/Meshes/SM_TowerServiceCore'))
for prop in ['left_arm','right_arm']:tower_defaults.get_editor_property(prop).set_static_mesh(u.load_asset(ROOT+'/Meshes/SM_CaptureArm'))
library.save_loaded_asset(tower_bp,False)
profile_path=ROOT+'/Data/DA_RecoveryMission'
if library.does_asset_exist(profile_path):profile=u.load_asset(profile_path)
else:
    factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.load_class(None,'/Script/SuperHeavySim.SuperHeavyRecoveryProfile'))
    profile=assets.create_asset('DA_RecoveryMission',ROOT+'/Data',None,factory);library.save_loaded_asset(profile)

# Rebuilding only this generated map is intentional; author custom sites as separate maps.
if library.does_asset_exist(LEVEL):levels.load_level(LEVEL)
else:levels.new_level(LEVEL)
for a in editor.get_all_level_actors():
    if a.get_actor_label().startswith('Recovery_'):editor.destroy_actor(a)

def actor(cls,name,loc=(0,0,0),rot=(0,0,0)):
    a=editor.spawn_actor_from_class(cls,u.Vector(*loc),u.Rotator(*rot));a.set_actor_label('Recovery_'+name)
    a.set_folder_path('Recovery/Site');return a
def shape(name,loc,size,mat,mesh='Cube',collision=False):
    a=actor(u.StaticMeshActor,name,loc);c=a.static_mesh_component
    c.set_static_mesh(u.load_asset('/Engine/BasicShapes/'+mesh));c.set_material(0,mat)
    a.set_actor_scale3d(u.Vector(*size));c.set_collision_enabled(u.CollisionEnabled.QUERY_AND_PHYSICS if collision else u.CollisionEnabled.NO_COLLISION)
    return a

shape('Ocean',(0,0,-350),(12000,12000,1),water,'Plane')
terrain=actor(u.StaticMeshActor,'CoastalShelf',(0,0,-220))
terrain.static_mesh_component.set_static_mesh(u.load_asset(ROOT+'/Meshes/SM_CoastalTerrain'))
terrain.static_mesh_component.set_material(0,green)
terrain.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
shape('FlightApron',(6500,0,-110),(430,260,2),concrete,collision=True)
shape('ServiceRoad',(4000,-9000,0),(700,14,0.06),asphalt)
for i in range(-18,24):shape('RoadDash'+str(i),(i*1500,-9000,5),(5,0.18,0.02),white)
for x in [-14500,27500]:shape('ApronBorder'+str(x),(x,0,0),(0.25,252,0.05),white)
for y in [-12500,12500]:shape('ApronBorder'+str(y),(6500,y,0),(420,0.25,0.05),white)
for x in range(-14000,27000,2000):shape('SlabJointX'+str(x),(x,0,-7),(0.04,258,0.02),joint)
for y in range(-12000,13000,2000):shape('SlabJointY'+str(y),(6500,y,-7),(428,0.04,0.02),joint)
for cx,title,radius in [(14000,'Launch',2300),(2400,'Catch',1600)]:
    shape(title+'Pad',(cx,0,-25),(radius*2/100,radius*2/100,0.5),asphalt,'Cylinder',True)
    for i in range(64):
        ang=i*math.tau/64
        a=shape(title+'Ring'+str(i),(cx+math.cos(ang)*radius,math.sin(ang)*radius,4),(0.16,1.5,0.025),white)
        a.set_actor_rotation(u.Rotator(0,math.degrees(ang),0),False)
    for s in [-1,1]:
        shape(title+'CrossX'+str(s),(cx+s*900,0,4),(5,0.15,0.03),amber)
        shape(title+'CrossY'+str(s),(cx,s*900,4),(0.15,5,0.03),amber)
for x in range(-12000,27000,2500):
    for y in [-12000,12000]:shape('PerimeterLight'+str(x)+str(y),(x,y,15),(0.6,0.6,0.15),glow)

tower=actor(tower_bp.generated_class(),'Tower')
tower.set_folder_path('Recovery/Mission')
tower.get_editor_property('structure').set_material(0,steel)
tower.get_editor_property('architectural_details').set_static_mesh(u.load_asset(ROOT+'/Meshes/SM_TowerServiceCore'))
for prop in ['left_arm','right_arm']:
    arm=tower.get_editor_property(prop);arm.set_static_mesh(u.load_asset(ROOT+'/Meshes/SM_CaptureArm'))
shape('TowerFoundation',(0,0,-100),(22,22,2),light,collision=True)
for i in range(5):
    x=-9500+i*2000
    shape('PropellantTank'+str(i),(x,7000,600),(12,12,12),light,'Cylinder')
    shape('TankCap'+str(i),(x,7000,1200),(12,12,3),steel,'Sphere')
    shape('TankBand'+str(i),(x,7000,1100),(12.1,12.1,0.5),amber,'Cylinder')
shape('OperationsBuilding',(-7000,-6000,600),(48,20,12),steel)
shape('OperationsRoof',(-7000,-6000,1220),(50,22,0.4),light)
for i in range(10):shape('OperationsWindow'+str(i),(-9200+i*450,-7010,700),(3,0.05,2),glow)
for i in range(4):
    x=-4000+i*3000
    shape('UtilityPipe'+str(i),(x,4200,90),(29,0.6,0.6),light)

for x in range(-14000,28000,800):
    for y in [-13500,13500]:
        shape('FencePost'+str(x)+str(y),(x,y,160),(0.12,0.12,3.2),light)
for y in [-13500,13500]:
    for z in [70,220,315]:shape('FenceRail'+str(y)+str(z),(6500,y,z),(420,0.06,0.06),light)
for i in range(6):
    x=-11500+i*1400
    shape('EquipmentContainer'+str(i),(x,10000,150),(12,3,3),steel)
    shape('EquipmentContainerRoof'+str(i),(x,10000,304),(12.1,3.1,0.08),light)
    for j in range(12):shape('ContainerRib'+str(i)+'_'+str(j),(x-550+j*100,9830,150),(0.045,0.12,3),light)

def sign(name,text,loc,rot,size):
    a=actor(u.TextRenderActor,name,loc,rot);c=a.text_render
    c.set_text(text);c.set_world_size(size);c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(u.Color(211,225,220,255));return a
sign('OperationsSign','RECOVERY LAB',(-7000,-7018,980),(0,-90,0),200)
sign('TowerIdentity','TOWER 01',(720,-20,9500),(0,0,0),110)
sign('LaunchIdentity','LZ - 01',(14000,-3400,8),(90,90,0),420)
sign('CaptureIdentity','CATCH',(2400,-2600,8),(90,90,0),240)

sun=actor(u.DirectionalLight,'Sun',(0,0,20000),(-22,-35,0))
sun.light_component.set_intensity(22000);sun.light_component.set_light_color(u.LinearColor(1,0.89,0.76))
sun.light_component.set_editor_property('atmosphere_sun_light',True)
actor(u.SkyAtmosphere,'Atmosphere')
sky=actor(u.SkyLight,'Sky')
sky.light_component.set_editor_property('real_time_capture',True);sky.light_component.set_intensity(1.1)
fog=actor(u.ExponentialHeightFog,'Fog',(0,0,-500))
fog.component.set_editor_property('fog_density',0.009)
fog.component.set_editor_property('fog_height_falloff',0.2)
post=actor(u.PostProcessVolume,'Exposure')
post.set_editor_property('unbound',True)
settings=post.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_min_brightness',True);settings.set_editor_property('auto_exposure_min_brightness',11)
settings.set_editor_property('override_auto_exposure_max_brightness',True);settings.set_editor_property('auto_exposure_max_brightness',14)
post.set_editor_property('settings',settings)
director=actor(director_bp.generated_class(),'MissionDirector')
director.set_folder_path('Recovery/Mission')
director.set_editor_property('tower',tower);director.set_editor_property('mission_profile',profile)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',u.load_class(None,'/Script/SuperHeavySim.SuperHeavyRecoveryGameMode'))
for a in editor.get_all_level_actors():
    if isinstance(a,(u.DirectionalLight,u.SkyLight,u.SkyAtmosphere,u.ExponentialHeightFog,u.PostProcessVolume)):a.set_folder_path('Recovery/Lighting')
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(23000,-30000,13500),u.Rotator(-16,130,0))
levels.save_current_level()
u.log('RECOVERY_SCENE_READY '+LEVEL)

"""Author flight presentation assets and update only generated Recovery actors."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
from unreal_materials import expression as expr, scalar, color, constant, custom, sample, save, connect

import unreal as u, os, math
A=u.EditorAssetLibrary; E=u.MaterialEditingLibrary; AT=u.AssetToolsHelpers.get_asset_tools()
ROOT='/Game/Starbase';out=str(ART_ROOT/'Flight')
def mat(name):
    p=ROOT+'/Materials/'+name
    m=u.load_asset(p) if A.does_asset_exist(p) else AT.create_asset(name,ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
    E.delete_all_material_expressions(m);return m
def opaque(name,c,metal=0,rough=0.5):
    m=mat(name);connect(m, color(m, c), u.MaterialProperty.MP_BASE_COLOR);connect(m, constant(m, metal), u.MaterialProperty.MP_METALLIC);connect(m, constant(m, rough), u.MaterialProperty.MP_ROUGHNESS);save(m);return m
NOISE='''struct NoiseLib {
 float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 float noise(float2 p) { float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y); }
 float fbm(float2 p) { return noise(p)*0.55+noise(p*2.03+17)*0.27+noise(p*4.17+41)*0.12+noise(p*8.37)*0.06; }
}; NoiseLib N;
'''
# Real water normal textures already supplied with this project, sampled at two scales.
water=mat('M_FlightOcean')
pos=expr(water,u.MaterialExpressionWorldPosition);time=expr(water,u.MaterialExpressionTime)
uv1=custom(water,{'P':pos,'T':time},'return P.xy/1600.0+float2(T*0.012,T*0.004);',u.CustomMaterialOutputType.CMOT_FLOAT2)
uv2=custom(water,{'P':pos,'T':time},'return P.xy/5300.0+float2(-T*0.004,T*0.006);',u.CustomMaterialOutputType.CMOT_FLOAT2)
tex=u.load_asset('/Game/ThirdParty/WaterMaterials/Textures/T_Ocean_Waves01_Normals')
samples=[]
for uv in [uv1,uv2]:
    s=expr(water,u.MaterialExpressionTextureSample);s.set_editor_property('texture',tex);s.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);assert E.connect_material_expressions(uv,'',s,E.get_material_expression_input_names(s)[0]);samples.append(s)
normal=custom(water,{'A':samples[0],'B':samples[1]},'return normalize(float3((A.xy+B.xy-1)*0.44,1));')
connect(water, normal, u.MaterialProperty.MP_NORMAL)
connect(water, color(water, (0.009, 0.05, 0.068)), u.MaterialProperty.MP_BASE_COLOR)
connect(water, constant(water, 0.24), u.MaterialProperty.MP_ROUGHNESS)
connect(water, constant(water, 0.25), u.MaterialProperty.MP_SPECULAR);save(water)
land=mat('M_FlightCoast');pos=expr(land,u.MaterialExpressionWorldPosition)
uvland=custom(land,{'P':pos},'return P.xy/2200;',u.CustomMaterialOutputType.CMOT_FLOAT2)
dirt=sample(land,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_Dirt_col',uvland)
noise=custom(land,{'P':pos,'Detail':dirt},NOISE+'float2 p=P.xy/130000; float n=N.fbm(p); float shore=60000+50000*sin(-P.y/1200000)+90000*sin(-P.y/2700000)-P.x; float beach=1-smoothstep(3500,24000,shore); float channel=1-smoothstep(0.02,0.055,abs(N.fbm(p*0.7)-0.49));float3 c=lerp(float3(0.13,0.15,0.065),float3(0.042,0.067,0.032),smoothstep(0.3,0.7,n));c=lerp(c,float3(0.022,0.045,0.042),channel*(1-beach)*0.75);c=lerp(c,float3(0.37,0.32,0.21),beach);return c*lerp(0.8,1.22,Detail.r);')
connect(land, sample(land, '/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_Dirt_nrm', uvland, True), u.MaterialProperty.MP_NORMAL)
connect(land, noise, u.MaterialProperty.MP_BASE_COLOR);connect(land, constant(land, 0.96), u.MaterialProperty.MP_ROUGHNESS);save(land)
ship=mat('M_UpperStage');uv=expr(ship,u.MaterialExpressionTextureCoordinate)
c=custom(ship,{'UV':uv},'float shield=step(0,sin(UV.x*6.28318)); float seams=1-0.16*pow(abs(sin(UV.y*54*3.14159)),60);float2 tile=frac(UV*float2(150,280));float grout=step(0.045,tile.x)*step(0.045,tile.y); return lerp(float3(0.44,0.47,0.49)*seams,float3(0.015,0.018,0.021)*(0.6+0.4*grout),shield);')
connect(ship, c, u.MaterialProperty.MP_BASE_COLOR);connect(ship, constant(ship, 0.7), u.MaterialProperty.MP_METALLIC);connect(ship, constant(ship, 0.32), u.MaterialProperty.MP_ROUGHNESS);save(ship)
flame=mat('M_RaptorPlume');flame.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE);flame.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT);flame.set_editor_property('two_sided',True)
uv=expr(flame,u.MaterialExpressionTextureCoordinate);th=scalar(flame,'Throttle',1);vac=scalar(flame,'Vacuum',0);t=scalar(flame,'FlightTime',0)
rgb=custom(flame,{'UV':uv,'T':t,'Throttle':th,'Vacuum':vac},'float z=UV.y; float turbulence=0.8+0.12*sin(z*103-T*47+sin(UV.x*31+T*7))+0.08*sin(z*241-T*63); float diamond=pow(max(0,sin(z*(55-20*Vacuum))),14)*(1-Vacuum); float3 col=lerp(float3(0.52,0.62,1),float3(1,0.29,0.075),smoothstep(0.3,0.9,z)); col=lerp(col,float3(1,0.9,0.78),diamond*0.85); return col*(15000+25000*diamond)*pow(saturate(1-z),1.4)*turbulence*Throttle;')
connect(flame, rgb, u.MaterialProperty.MP_EMISSIVE_COLOR)
opacity=custom(flame,{'UV':uv,'T':t},'return (0.38+0.18*sin(UV.y*79-T*23+sin(UV.x*25)))*smoothstep(0,0.025,UV.y)*pow(saturate(1-UV.y),0.5);',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(flame, opacity, u.MaterialProperty.MP_OPACITY);save(flame)
vapor=mat('M_GroundVapor');vapor.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT);vapor.set_editor_property('two_sided',True)
vapor.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_VOLUMETRIC_DIRECTIONAL)
tint=expr(vapor,u.MaterialExpressionVectorParameter);tint.set_editor_property('parameter_name','Tint');tint.set_editor_property('default_value',u.LinearColor(0.6,0.65,0.7,1));connect(vapor, tint, u.MaterialProperty.MP_BASE_COLOR)
uv=expr(vapor,u.MaterialExpressionTextureCoordinate);age=scalar(vapor,'Age',0);seed=scalar(vapor,'Seed',0);opacity=scalar(vapor,'Opacity',0.3)
density=custom(vapor,{'UV':uv,'Age':age,'Seed':seed,'Opacity':opacity},NOISE+'float2 q=UV*2-1;float n=N.fbm(UV*5+float2(Seed,Age*0.08));float edge=1-smoothstep(0.25,0.95,length(q)+(n-0.5)*0.36);return saturate(edge*(0.38+0.85*n)*Opacity);',u.CustomMaterialOutputType.CMOT_FLOAT1)
fade=expr(vapor,u.MaterialExpressionDepthFade);fade.set_editor_property('fade_distance_default',250)
assert E.connect_material_expressions(density,'',fade,'Opacity'),'Smoke opacity connection failed'
connect(vapor, fade, u.MaterialProperty.MP_OPACITY)
connect(vapor, constant(vapor, 1), u.MaterialProperty.MP_ROUGHNESS);save(vapor)
# Fin lattice has solid geometry; retain its baked normal detail with a darker,
# rough metal response rather than bright flat white surfaces.
fin=mat('M_GridFinAlloy');uv=expr(fin,u.MaterialExpressionTextureCoordinate)
normal=sample(fin,'/Game/Starbase/Vehicle/Textures/Superheavy-V3_Normal',uv,True)
connect(fin, normal, u.MaterialProperty.MP_NORMAL)
tone=custom(fin,{'UV':uv},NOISE+'float n=N.fbm(UV*230);return float3(0.24,0.255,0.26)*(0.84+0.22*n);')
connect(fin, tone, u.MaterialProperty.MP_BASE_COLOR);connect(fin, constant(fin, 0.88), u.MaterialProperty.MP_METALLIC)
connect(fin, constant(fin, 0.43), u.MaterialProperty.MP_ROUGHNESS);save(fin)
for name,base,rough in [('M_Concrete',(0.22,0.23,0.22),0.88),('M_Asphalt',(0.026,0.031,0.034),0.92)]:
    m=mat(name);p=expr(m,u.MaterialExpressionWorldPosition)
    c=custom(m,{'P':p,'Base':color(m,base)},NOISE+'float grain=N.noise(P.xy*0.6);float stain=N.fbm(P.xy/1400);return Base*(0.72+0.38*stain+0.14*grain);')
    connect(m, c, u.MaterialProperty.MP_BASE_COLOR);connect(m, constant(m, rough), u.MaterialProperty.MP_ROUGHNESS);save(m)
for name in ['SM_CurvedOcean','SM_CoastalPlain','SM_ExhaustEnvelope','SM_UpperStageProxy','SM_LaunchMount']:
    task=u.AssetImportTask();task.filename=os.path.join(out,name+'.fbx');task.destination_path=ROOT+'/Meshes';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
    opts=u.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False;opts.set_editor_property('automated_import_should_detect_type',False);opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;opts.static_mesh_import_data.combine_meshes=True;task.options=opts;AT.import_asset_tasks([task])
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);levels.load_level(ROOT+'/Maps/L_RecoveryLab')
ed=u.get_editor_subsystem(u.EditorActorSubsystem)
actors=ed.get_all_level_actors()
for a in actors:
    label=a.get_actor_label()
    if label in ['Recovery_Ocean','Recovery_FlightOcean','Recovery_FlightCoast','Recovery_CoastalShelf','Recovery_Clouds','Recovery_LaunchMount']:ed.destroy_actor(a)
    elif label=='Recovery_Sun':
        a.set_actor_rotation(u.Rotator(-38,-58,0),False);a.light_component.set_intensity(95000);a.light_component.set_light_color(u.LinearColor(1,0.975,0.94));a.light_component.set_editor_property('cast_cloud_shadows',True)
    elif label=='Recovery_Fog':
        a.component.set_editor_property('fog_density',0.0015);a.component.set_editor_property('enable_volumetric_fog',True);a.component.set_editor_property('volumetric_fog_distance',24000)
    elif label=='Recovery_Exposure':
        s=a.get_editor_property('settings')
        for k,v in [('auto_exposure_min_brightness',14.5),('auto_exposure_max_brightness',14.5),('bloom_intensity',0.24),('motion_blur_amount',0.08)]:
            s.set_editor_property('override_'+k,True);s.set_editor_property(k,v)
        a.set_editor_property('settings',s)
def mesh_actor(label,name,material):
    a=ed.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0));a.set_actor_label(label);a.set_folder_path('Recovery/Environment')
    a.static_mesh_component.set_static_mesh(u.load_asset(ROOT+'/Meshes/'+name));a.static_mesh_component.set_material(0,material);a.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);a.static_mesh_component.set_cast_shadow(False);return a
mesh_actor('Recovery_FlightOcean','SM_CurvedOcean',water)
mesh_actor('Recovery_FlightCoast','SM_CoastalPlain',land)
mount=mesh_actor('Recovery_LaunchMount','SM_LaunchMount',u.load_asset(ROOT+'/Materials/M_Graphite'))
mount.set_actor_location(u.Vector(14000,0,0),False,False);mount.static_mesh_component.set_cast_shadow(True)
cloud=ed.spawn_actor_from_class(u.VolumetricCloud,u.Vector(0,0,0));cloud.set_actor_label('Recovery_Clouds');cloud.set_folder_path('Recovery/Lighting')
cc=cloud.get_component_by_class(u.VolumetricCloudComponent)
cc.set_editor_property('material',u.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst'))
cc.set_editor_property('layer_bottom_altitude',1.8);cc.set_editor_property('layer_height',2.4);cc.set_editor_property('tracing_max_distance',300);cc.set_editor_property('view_sample_count_scale',1.5)
levels.save_current_level()
p=u.load_asset(ROOT+'/Data/DA_RecoveryMission')
p.set_editor_property('catch_lug_plus_m',u.Vector(4.99,0.0079,62.7978))
p.set_editor_property('catch_lug_minus_m',u.Vector(-4.99,0.0079,62.7978))
p.set_editor_property('capture_radius_m',0.35)
A.save_loaded_asset(p,False)
for a in ed.get_all_level_actors():
    if a.get_actor_label()=='Recovery_Tower':a.set_editor_property('arm_contact_height_above_base_m',61.9478)
bp=u.load_asset(ROOT+'/Blueprints/BP_LaunchTower')
u.get_default_object(bp.generated_class()).set_editor_property('arm_contact_height_above_base_m',61.9478)
A.save_loaded_asset(bp,False)
levels.save_current_level()
print('FLIGHT_PRESENTATION_READY')
exec(compile(open(os.path.join(u.Paths.project_dir(),'Tools','Editor','build_recovery_vfx.py'),encoding='utf-8').read(),'build_recovery_vfx.py','exec'))

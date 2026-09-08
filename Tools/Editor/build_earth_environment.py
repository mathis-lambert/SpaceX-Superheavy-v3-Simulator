"""Unreal authoring: geographic Earth, Starbase terrain, atmosphere and site dressing.
Only replaces generated Earth/environment actors; leaves tower and flight assets intact.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
from unreal_materials import prop, expression as ex, constant, custom, sample, connect, save

import unreal as u,os,json,math
from pathlib import Path
ROOT='/Game/Starbase';SRC=ART_ROOT/'Earth'
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary;AT=u.AssetToolsHelpers.get_asset_tools()
def import_asset(name,folder,file,mesh=False):
    path=ROOT+'/'+folder+'/'+name
    if A.does_asset_exist(path) and not(mesh and '-EarthReimport' in u.SystemLibrary.get_command_line()):return u.load_asset(path)
    task=u.AssetImportTask();task.filename=str(SRC/file);task.destination_path=ROOT+'/'+folder;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=False
    if mesh:
        opts=u.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
        prop(opts,'automated_import_should_detect_type',False);opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
        opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.generate_lightmap_u_vs=False
        opts.static_mesh_import_data.auto_generate_collision=False;task.options=opts
    AT.import_asset_tasks([task]);asset=u.load_asset(path);assert asset,path
    return asset
textures={}
for name,file in [('T_EarthSeptember','Earth_September.jpg'),('T_GulfRegion','GulfRegion.jpg'),('T_BocaRegion','BocaRegion.png')]+[(f'T_BocaChica_{i}_{j}',f'BocaChica_{i}_{j}.png') for j in range(4) for i in range(4)]:
    t=import_asset(name,'Textures/Earth',file);prop(t,'srgb',True)
    if name=='T_EarthSeptember':
        prop(t,'power_of_two_mode',u.TexturePowerOfTwoSetting.RESIZE_TO_SPECIFIC_RESOLUTION)
        prop(t,'resize_during_build_x',8192);prop(t,'resize_during_build_y',4096);prop(t,'max_texture_size',8192)
        prop(t,'virtual_texture_streaming',False)
    else:prop(t,'address_x',u.TextureAddress.TA_CLAMP);prop(t,'address_y',u.TextureAddress.TA_CLAMP)
    if name=='T_BocaRegion':
        prop(t,'power_of_two_mode',u.TexturePowerOfTwoSetting.RESIZE_TO_SPECIFIC_RESOLUTION)
        prop(t,'resize_during_build_x',4096);prop(t,'resize_during_build_y',4096)
    prop(t,'compression_settings',u.TextureCompressionSettings.TC_BC7)
    prop(t,'mip_gen_settings',u.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    A.save_loaded_asset(t,False);textures[name]=t
def material(name):
    p=ROOT+'/Materials/Earth/'+name
    m=u.load_asset(p) if A.does_asset_exist(p) else AT.create_asset(name,ROOT+'/Materials/Earth',u.Material,u.MaterialFactoryNew())
    E.delete_all_material_expressions(m);prop(m,'used_with_nanite',True);return m
def make_surface(name,tex,kind):
    m=material(name);uv=ex(m,u.MaterialExpressionTextureCoordinate);pos=ex(m,u.MaterialExpressionWorldPosition)
    base=sample(m,tex,uv)
    # Geographic UV in the origin's east/north/up basis; same mapping at all scales.
    geo=custom(m,{'P':(pos,'')},'''float3 q=normalize(float3(P.xy,P.z+637100000.0));
    float3 e=float3(0.992208696,-0.124586893,0);float3 n=float3(0.054610022,0.434913642,0.898814703);float3 up=float3(-0.111980532,-0.891811774,0.438328791);
    float3 p=e*q.x+n*q.y+up*q.z;return float2(atan2(p.y,p.x)/6.283185307+0.5,0.5-asin(clamp(p.z,-1,1))/3.141592654);''',u.CustomMaterialOutputType.CMOT_FLOAT2)
    if kind!='globe':
        if kind in ['local','basin']:
            regionuv=custom(m,{'G':(geo,'')},'return float2((G.x*360-180+109.1569)/24,(37.9973-(90-G.y*180))/24);',u.CustomMaterialOutputType.CMOT_FLOAT2)
            fallback=sample(m,textures['T_GulfRegion'],regionuv)
        else:fallback=sample(m,textures['T_EarthSeptember'],geo)
        fallback_pin='RGB'
        if kind=='local':
            lat_half=math.degrees(60000/6371000);lon_half=lat_half/math.cos(math.radians(25.9973))
            basinuv=custom(m,{'G':(geo,'')},f'return float2((G.x*360-180+97.1569+{lon_half})/{2*lon_half},(25.9973+{lat_half}-(90-G.y*180))/{2*lat_half});',u.CustomMaterialOutputType.CMOT_FLOAT2)
            mid=sample(m,textures['T_BocaRegion'],basinuv)
            fallback=custom(m,{'C':(mid,'RGB'),'A':(mid,'A'),'B':(fallback,'RGB')},'return lerp(B,C,A);');fallback_pin=''
        inputs={'C':(base,'RGB'),'B':(fallback,fallback_pin),'UV':(uv,''),'P':(pos,'')}
        if kind=='local':
            # Alpha marks NAIP gaps. The photograph remains spatially registered.
            inputs['A']=(base,'A');code='float edge=600000-max(abs(P.x),abs(P.y)); float w=A*saturate(edge/60000);'
        elif kind=='basin':
            inputs['A']=(base,'A');code='float edge=6000000-max(abs(P.x),abs(P.y)); float w=A*saturate(edge/600000);'
        else:code='float edge=min(min(UV.x,1-UV.x),min(UV.y,1-UV.y));float w=saturate(edge*18);'
        base=custom(m,inputs,code+'return lerp(B,C,w);')
    if kind=='local':
        detailuv=custom(m,{'P':(pos,'')},'return P.xy/450;',u.CustomMaterialOutputType.CMOT_FLOAT2)
        sand=sample(m,u.load_asset('/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_col'),detailuv)
        tone=custom(m,{'C':(base,''),'D':(sand,'RGB'),'P':(pos,'')},'float near=1-saturate(length(P.xy)/350000);return C*lerp(1.0,0.76+dot(D,float3(.299,.587,.114))*.7,near*.45);')
        connect(m,tone,u.MaterialProperty.MP_BASE_COLOR)
        normal=sample(m,u.load_asset('/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_nrm'),detailuv,True)
        time=ex(m,u.MaterialExpressionTime)
        waveuv=custom(m,{'P':(pos,''),'T':(time,'')},'return P.xy/2100+float2(T*.008,T*.003);',u.CustomMaterialOutputType.CMOT_FLOAT2)
        wave=sample(m,u.load_asset('/Game/ThirdParty/WaterMaterials/Textures/T_Ocean_Waves01_Normals'),waveuv)
        prop(wave,'sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        mask=custom(m,{'P':(pos,''),'C':(base,'')},'float h=P.z+dot(P.xy,P.xy)/1274200000.;return (1-smoothstep(-405,-330,h))*(1-smoothstep(.10,.18,dot(C,float3(.299,.587,.114))));',u.CustomMaterialOutputType.CMOT_FLOAT1)
        normals=custom(m,{'N':(normal,'RGB'),'W':(wave,'RGB'),'A':(mask,''),'P':(pos,'')},'float e=saturate((600000-max(abs(P.x),abs(P.y)))/100000);return normalize(lerp(float3(0,0,1),lerp(N,float3((W.xy*2-1)*.2,1),A),e));')
        connect(m,normals,u.MaterialProperty.MP_NORMAL)
        rough=custom(m,{'A':(mask,''),'P':(pos,'')},'float e=saturate((600000-max(abs(P.x),abs(P.y)))/100000);return lerp(.55,lerp(.84,.32,A),e);',u.CustomMaterialOutputType.CMOT_FLOAT1)
        connect(m,rough,u.MaterialProperty.MP_ROUGHNESS)
    else:connect(m,base,u.MaterialProperty.MP_BASE_COLOR,'RGB' if kind=='globe' else '')
    if kind!='local':connect(m,constant(m,.55),u.MaterialProperty.MP_ROUGHNESS)
    save(m);return m
mats={'SM_EarthGlobe':make_surface('M_EarthGlobe',textures['T_EarthSeptember'],'globe'),'SM_GulfRegion':make_surface('M_GulfRegion',textures['T_GulfRegion'],'region'),'SM_BocaRegion':make_surface('M_BocaRegion',textures['T_BocaRegion'],'basin')}
for j in range(4):
    for i in range(4):mats[f'SM_BocaChica_{i}_{j}']=make_surface(f'M_BocaChica_{i}_{j}',textures[f'T_BocaChica_{i}_{j}'],'local')
meshes={}
for name in mats:
    mesh=import_asset(name,'Meshes/Earth',name+'.fbx',True);mesh.set_material(0,mats[name])
    settings=mesh.get_editor_property('nanite_settings');prop(settings,'enabled',name.startswith('SM_BocaChica'))
    if name.startswith('SM_BocaChica'):prop(settings,'position_precision',2)
    prop(mesh,'nanite_settings',settings)
    A.save_loaded_asset(mesh,False);meshes[name]=mesh
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);levels.load_level(ROOT+'/Maps/L_RecoveryLab');ed=u.get_editor_subsystem(u.EditorActorSubsystem)
for actor in ed.get_all_level_actors():
    label=actor.get_actor_label()
    if label.startswith('Recovery_Earth_') or label in ['Recovery_FlightOcean','Recovery_FlightCoast','Recovery_Ocean','Recovery_CoastalShelf']:
        ed.destroy_actor(actor)
def spawn(name,mesh,mat=None,xyz=(0,0,0),scale=(1,1,1),shadow=False):
    a=ed.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*[v*100 for v in xyz]));a.set_actor_label('Recovery_Earth_'+name);a.set_folder_path('Recovery/Earth')
    c=a.static_mesh_component;c.set_static_mesh(mesh)
    if mat:c.set_material(0,mat)
    c.set_collision_profile_name('NoCollision',False);c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);c.set_cast_shadow(shadow)
    if name.startswith('SM_'):prop(c,'affect_distance_field_lighting',False);prop(c,'visible_in_ray_tracing',False)
    a.set_actor_scale3d(u.Vector(*scale));return a
for name,mesh in meshes.items():
    bounds=mesh.get_bounds();print('EARTH_BOUNDS',name,bounds.origin)
    # FBX converts Blender's right-handed Y to Unreal's left-handed coordinate.
    spawn(name,mesh,scale=(1,-1,1))
for actor in ed.get_all_level_actors():
    label=actor.get_actor_label()
    if label=='Recovery_Sun':
        actor.set_actor_rotation(u.Rotator(pitch=-32,yaw=138,roll=0),False);c=actor.light_component;c.set_intensity(100000)
        prop(c,'per_pixel_atmosphere_transmittance',True);prop(c,'cast_cloud_shadows',True)
        prop(c,'cloud_shadow_strength',.35)
    if isinstance(actor,u.SkyAtmosphere):
        c=actor.get_component_by_class(u.SkyAtmosphereComponent)
        prop(c,'transform_mode',u.SkyAtmosphereTransformMode.PLANET_CENTER_AT_COMPONENT_TRANSFORM)
        actor.set_actor_location(u.Vector(0,0,-637100000),False,False)
        # Put the analytical ground below the chords of the finite-resolution
        # sphere. Otherwise per-pixel sunlight treats terrain as underground.
        prop(c,'bottom_radius',6370.5);prop(c,'atmosphere_height',100.5)
        prop(c,'rayleigh_scattering',u.LinearColor(r=.005802,g=.013558,b=.0331,a=1.))
        prop(c,'rayleigh_scattering_scale',1.);prop(c,'rayleigh_exponential_distribution',8.)
        prop(c,'mie_scattering',u.LinearColor(r=.003996,g=.003996,b=.003996,a=1.))
        prop(c,'mie_absorption',u.LinearColor(r=.000444,g=.000444,b=.000444,a=1.))
        prop(c,'mie_scattering_scale',.65);prop(c,'mie_absorption_scale',1.)
        prop(c,'mie_exponential_distribution',1.2);prop(c,'trace_sample_count_scale',2.)
    if label=='Recovery_Fog':
        prop(actor.component,'fog_density',.00055);prop(actor.component,'fog_height_falloff',.3);prop(actor.component,'volumetric_fog_distance',50000)
    if label=='Recovery_Clouds':
        c=actor.get_component_by_class(u.VolumetricCloudComponent);prop(c,'planet_radius',6371.)
        prop(c,'layer_bottom_altitude',2.2);prop(c,'layer_height',2.8);prop(c,'tracing_max_distance',500.)
        prop(c,'view_sample_count_scale',2.);prop(c,'shadow_view_sample_count_scale',1.5)
    if label=='Recovery_Exposure':
        s=actor.get_editor_property('settings')
        for k,v in [('auto_exposure_min_brightness',14.),('auto_exposure_max_brightness',14.),('bloom_intensity',.25)]:prop(s,'override_'+k,True);prop(s,k,v)
        prop(actor,'settings',s)
# Editable transforms sampled against terrain rather than random flat placement.
path=ROOT+'/Data/DA_RecoveryEnvironment'
if A.does_asset_exist(path):profile=u.load_asset(path)
else:
    factory=u.DataAssetFactory();prop(factory,'data_asset_class',u.RecoveryEnvironmentProfile)
    profile=AT.create_asset('DA_RecoveryEnvironment',ROOT+'/Data',u.RecoveryEnvironmentProfile,factory)
data=json.loads((SRC/'scenery.json').read_text())
for field in ['grass','rocks']:
    transforms=[]
    for x,y,z,yaw,s in data[field]:transforms.append(u.Transform(location=u.Vector(x*100,y*100,z*100),rotation=u.Rotator(yaw=yaw),scale=u.Vector(s,s,s)))
    prop(profile,field,transforms)
A.save_loaded_asset(profile,False)
# Additional service infrastructure with articulated volumes at the platform edge.
cube=u.load_asset('/Engine/BasicShapes/Cube');cylinder=u.load_asset('/Engine/BasicShapes/Cylinder')
steel=u.load_asset(ROOT+'/Materials/M_Cladding');white=u.load_asset(ROOT+'/Materials/M_Concrete');dark=u.load_asset(ROOT+'/Materials/M_Graphite');amber=u.load_asset(ROOT+'/Materials/M_SafetyAmber')
for i in range(3):
    x,y=-115+i*43,114
    spawn(f'Compressor_{i}',cube,steel,(x,y,2),(28,10,4),True)
    for k in range(3):spawn(f'CompressorVent_{i}_{k}',cylinder,dark,(x-8+k*8,y,4.15),(4,4,.3),True)
    for k in range(8):spawn(f'Panel_{i}_{k}',cube,dark,(x-12+k*3.5,y-5.02,2),(2.5,.06,2.9))
for i in range(10):
    x,y=-134+i*40,-116
    spawn(f'Barrier_{i}',cube,white,(x,y,.5),(8,.6,1),True)
    spawn(f'BarrierStripe_{i}',cube,amber,(x,y-.32,.6),(6,.03,.2))
for i in range(4):
    x,y=-142, -35+i*20
    spawn(f'UtilitySkid_{i}',cube,dark,(x,y,.4),(6,11,.8),True)
    for k in range(2):spawn(f'UtilityVessel_{i}_{k}',cylinder,steel,(x-1.7+k*3.4,y,2.5),(2.5,2.5,4.5),True)
levels.save_current_level()
result=dict(success=True,globe_radius_m=6371000,origin=[25.9973,-97.1569],earth_meshes=len(meshes),textures=len(textures),grass=len(data['grass']),rocks=len(data['rocks']))
(Path(u.Paths.project_saved_dir())/'Recovery'/'earth-assets.json').write_text(json.dumps(result,indent=2))
print('EARTH_ENVIRONMENT_READY',result)
exec(compile((Path(u.Paths.project_dir())/'Tools'/'Tests'/'audit_earth_assets.py').read_text(encoding='utf-8'),'audit_earth_assets.py','exec'))

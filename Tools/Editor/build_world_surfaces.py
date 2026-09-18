"""Canonical geographic shading across all Earth geometry scales.

Uses the authored level and registered terrain assets. Geometry coverage no
longer controls imagery coverage: all shells use the same geodetic UVs, water
BRDF and fallback blending.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from project_paths import ART_ROOT, SAVED_ROOT
from unreal_materials import u, E, A, prop, expression as ex, material, custom, sample, scalar, vector, constant, color, connect, save
from water_surface import normal_code, displacement_code
import json
import math

ROOT='/Game/Starbase'
tools=u.AssetToolsHelpers.get_asset_tools()
def import_texture(name,path,srgb=True,compression=u.TextureCompressionSettings.TC_BC7):
    existing=u.load_asset(ROOT+'/Textures/Earth/'+name)
    command_line=u.SystemLibrary.get_command_line()
    refresh_coast='-CoastalShadingReimport' in command_line and name=='T_CoastalHydrology'
    if existing and '-WorldReimportTextures' not in command_line and not refresh_coast:return existing
    task=u.AssetImportTask();task.filename=str(path);task.destination_path=ROOT+'/Textures/Earth';task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    tools.import_asset_tasks([task]);t=u.load_asset(task.destination_path+'/'+name);assert t,name
    prop(t,'srgb',srgb);prop(t,'compression_settings',compression)
    prop(t,'address_x',u.TextureAddress.TA_CLAMP);prop(t,'address_y',u.TextureAddress.TA_CLAMP)
    prop(t,'never_stream',False);prop(t,'lod_bias',0);prop(t,'max_texture_size',8192)
    prop(t,'mip_gen_settings',u.TextureMipGenSettings.TMGS_SHARPEN1);assert A.save_loaded_asset(t,False)
    return t

coast=import_texture('T_CoastContinuous',ART_ROOT/'Earth/Continuity/Coast8192.png')
gulf=import_texture('T_GulfContinuous',ART_ROOT/'Earth/Continuity/Gulf8192.png')
regional=import_texture('T_RegionalContinuous',ART_ROOT/'Earth/Continuity/Regional8192.png')
globe=u.load_asset(ROOT+'/Textures/Earth/T_EarthSeptember')
hydrology=import_texture('T_CoastalHydrology',ART_ROOT/'Earth/Continuity/CoastalHydrology.png',False,u.TextureCompressionSettings.TC_MASKS)
global_water=import_texture('T_GlobalOceanMask',ART_ROOT/'Earth/WaterCoverage/GlobalOcean.png',False,u.TextureCompressionSettings.TC_MASKS)
regional_water=import_texture('T_RegionalOceanMask',ART_ROOT/'Earth/WaterCoverage/RegionalOcean.png',False,u.TextureCompressionSettings.TC_MASKS)
prop(global_water,'address_x',u.TextureAddress.TA_WRAP);A.save_loaded_asset(global_water,False)
GEO='''float3 q=normalize(float3(P.xy,P.z+637100000.0));
float3 e=float3(.992208696,-.124586893,0),n=float3(.054610022,.434913642,.898814703),up=float3(-.111980532,-.891811774,.438328791);
float3 p=e*q.x+n*q.y+up*q.z;
return float2(atan2(p.y,p.x)*57.295779513,asin(clamp(p.z,-1,1))*57.295779513);'''

def build_surface(name,tile=None):
    m=material(ROOT+'/Materials/Earth/'+name)
    prop(m,'tangent_space_normal',False)
    p=ex(m,u.MaterialExpressionWorldPosition);camera=ex(m,u.MaterialExpressionCameraPositionWS);time=ex(m,u.MaterialExpressionTime)
    geo=custom(m,{'P':p},GEO,u.CustomMaterialOutputType.CMOT_FLOAT2)
    globeuv=custom(m,{'G':geo},'return float2((G.x+180)/360,(90-G.y)/180);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    global_color=sample(m,globe,globeuv)
    gulfuv=custom(m,{'G':geo},'return float2((G.x+109.1569)/24,(37.9973-G.y)/24);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    coastuv=custom(m,{'G':geo},'return float2((G.x+98.3569)/2.4,(27.1973-G.y)/2.4);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    region=sample(m,gulf,gulfuv);local=sample(m,coast,coastuv)
    def blend(base,detail,uv):
        return custom(m,{'B':base,'C':detail,'UV':uv},'float edge=min(min(UV.x,1-UV.x),min(UV.y,1-UV.y));return lerp(B,C,smoothstep(0,.08,edge));')
    base=blend((global_color,'RGB'),(region,'RGB'),gulfuv)
    base=blend(base,(local,'RGB'),coastuv)
    regionaluv=custom(m,{'G':geo},'return float2((G.x+97.4569)/.6,(26.2973-G.y)/.6);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    regional_color=sample(m,regional,regionaluv)
    base=blend(base,(regional_color,'RGB'),regionaluv)
    ocean_global=sample(m,global_water,globeuv,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
    ocean_region=sample(m,regional_water,gulfuv,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
    water=custom(m,{'Global':(ocean_global,'R'),'Region':(ocean_region,'R'),'UV':gulfuv},
        'float edge=min(min(UV.x,1-UV.x),min(UV.y,1-UV.y));return lerp(Global,Region,smoothstep(0,.08,edge));',u.CustomMaterialOutputType.CMOT_FLOAT1)
    half_lat=math.degrees(6000/6371000);half_lon=half_lat/math.cos(math.radians(25.9973))
    maskuv=custom(m,{'G':geo},f'return float2((G.x-({-97.1569-half_lon:.12f}))/{half_lon*2:.12f},({25.9973+half_lat:.12f}-G.y)/{half_lat*2:.12f});',u.CustomMaterialOutputType.CMOT_FLOAT2)
    hydro=sample(m,hydrology,maskuv,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
    coverage=custom(m,{'UV':maskuv},'return smoothstep(0,.05,min(min(UV.x,1-UV.x),min(UV.y,1-UV.y)));',u.CustomMaterialOutputType.CMOT_FLOAT1)
    water=custom(m,{'W':water,'H':hydro,'Coverage':coverage},'return lerp(W,H.r,Coverage);',u.CustomMaterialOutputType.CMOT_FLOAT1)
    if tile is not None:
        i,j=tile;half_lat=math.degrees(6000/6371000);half_lon=half_lat/math.cos(math.radians(25.9973))
        lo=-97.1569-half_lon+i*half_lon/2;hi=25.9973-half_lat+(j+1)*half_lat/2
        tileuv=custom(m,{'G':geo},f'return float2((G.x-({lo:.12f}))/{half_lon/2:.12f},({hi:.12f}-G.y)/{half_lat/2:.12f});',u.CustomMaterialOutputType.CMOT_FLOAT2)
        aerial=sample(m,ROOT+f'/Textures/Earth/T_BocaChica_{i}_{j}',tileuv)
        coarse=sample(m,ROOT+f'/Textures/Earth/T_BocaChica_{i}_{j}',tileuv)
        prop(coarse,'mip_value_mode',u.TextureMipValueMode.TMVM_MIP_LEVEL);prop(coarse,'const_mip_value',8)
        regional_coarse=sample(m,regional,regionaluv)
        prop(regional_coarse,'mip_value_mode',u.TextureMipValueMode.TMVM_MIP_LEVEL);prop(regional_coarse,'const_mip_value',4)
        base=custom(m,{'B':base,'C':(aerial,'RGB'),'Low':(coarse,'RGB'),'Reference':(regional_coarse,'RGB'),'A':(aerial,'A'),'UV':tileuv,'P':p,'Camera':camera},'''
float edge=600000-max(abs(P.x),abs(P.y));
float near=1-smoothstep(1000000,3000000,length(Camera-P));
// Match broad photographic exposure while retaining the aerial high frequencies.
float3 matched=C*clamp(Reference/max(Low,float3(.015,.015,.015)),.65,1.5);
float tileEdge=min(min(UV.x,1-UV.x),min(UV.y,1-UV.y));
return lerp(B,matched,A*smoothstep(0,120000,edge)*near*smoothstep(0,.025,tileEdge));''')
    # Water comes from a BRDF, not a lit photograph of an old ocean surface.
    base=custom(m,{'C':base,'W':water,'H':hydro,'Coverage':coverage},'''float shallow=Coverage*exp(-H.b*7);
float3 sea=lerp(float3(.004,.018,.028),float3(.022,.067,.067),shallow);
float shoreDistance=(H.g-.5)*256;
float wetSand=Coverage*(1-W)*exp(-abs(shoreDistance)/6);
return lerp(C*(1-wetSand*.24),sea,W);''')
    grounduv=custom(m,{'P':p},'return P.xy/430;',u.CustomMaterialOutputType.CMOT_FLOAT2)
    grain=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_col',grounduv)
    secondaryuv=custom(m,{'P':p},'return float2(P.x*.73+P.y*.683,-P.x*.683+P.y*.73)/1130+float2(.37,.81);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    secondarygrain=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_col',secondaryuv)
    grainmix=custom(m,{'A':(grain,'RGB'),'B':(secondarygrain,'RGB'),'P':p},'float weight=.5+.2*sin(P.x*.00013+sin(P.y*.00021));return lerp(A,B,weight);')
    ground=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_nrm',grounduv,normal=True)
    base=custom(m,{'C':base,'D':grainmix,'W':water,'P':p,'Camera':camera},'''float detail=(1-W)*(1-smoothstep(15000,150000,length(Camera-P)));
float grain=clamp(dot(D,float3(.299,.587,.114))*2.6,.5,1.6);
float close=(1-W)*(1-smoothstep(4000,40000,length(Camera-P)));
float3 tint=C/max(.08,dot(C,float3(.299,.587,.114)));
float3 granular=clamp(tint,float3(.6,.6,.6),float3(1.4,1.4,1.4))*D*1.7;
return lerp(C*lerp(1.,grain,detail*.8),granular,close*.9);''')
    connect(m,base,u.MaterialProperty.MP_BASE_COLOR)
    surf=custom(m,{'C':base,'W':water,'H':hydro,'Coverage':coverage,'P':p,'Camera':camera,'T':time},'''
float distance=(H.g-.5)*256;
float crest=smoothstep(.5,.97,sin(distance*.24+T*1.25+sin(P.y*.003)*.7));
float visible=1-smoothstep(20000,150000,length(Camera-P));
float breakup=.5+.24*sin(P.x*.012+sin(P.y*.019))+.26*sin(P.y*.032+P.x*.017);
float foam=W*Coverage*(1-smoothstep(3,24,abs(distance)))*crest*smoothstep(.32,.8,breakup)*visible;
return lerp(C,float3(.52,.57,.56),foam*.5);''')
    connect(m,surf,u.MaterialProperty.MP_BASE_COLOR)
    # Vertex deformation only needs the surveyed local water mask. Passing the
    # pixel-stage water classification here also samples every orbital imagery
    # layer per vertex, including in the velocity pass. Outside this local patch
    # the ocean keeps its wave normals; small vertex swells are not evaluated.
    swell=custom(m,{'P':p,'T':time,'Camera':camera,'H':hydro,'Coverage':coverage},displacement_code())
    connect(m,swell,u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    vertex=ex(m,u.MaterialExpressionVertexNormalWS)
    normal=custom(m,{'P':p,'Camera':camera,'T':time,'W':water,'Terrain':vertex,'Ground':(ground,'RGB')},normal_code())
    connect(m,normal,u.MaterialProperty.MP_NORMAL)
    rough=custom(m,{'W':water,'P':p,'Camera':camera,'H':hydro,'Coverage':coverage},'''float wetSand=Coverage*(1-W)*exp(-abs((H.g-.5)*256)/6);
return lerp(lerp(.86,.42,wetSand),lerp(.17,.27,smoothstep(10000,650000,length(Camera-P))),W);''',u.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(m,rough,u.MaterialProperty.MP_ROUGHNESS)
    connect(m,constant(m,.38),u.MaterialProperty.MP_SPECULAR)
    save(m)
    return m

for name in ('M_EarthGlobe','M_GulfRegion','M_BocaRegion'):build_surface(name)
for j in range(4):
    for i in range(4):build_surface(f'M_BocaChica_{i}_{j}',(i,j))

# The independent starfield overlay is authored by build_starfield_overlay.py.
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);assert levels.load_level(ROOT+'/Maps/L_RecoveryLab')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='Recovery_Clouds':
        cloud=a.get_component_by_class(u.VolumetricCloudComponent)
        prop(cloud,'tracing_start_max_distance',1800.)
        prop(cloud,'tracing_max_distance_mode',u.VolumetricCloudTracingMaxDistanceMode.DISTANCE_FROM_CLOUD_LAYER_ENTRY_POINT)
        prop(cloud,'tracing_max_distance',2200.)
    if a.get_actor_label()=='Recovery_Sun':
        prop(a.light_component,'cloud_shadow_extent',150.)
levels.save_current_level()
report=dict(success=True,materials=19,shared_geographic_contract=True,coast_pixels=8192,gulf_pixels=8192,regional_pixels=8192,regional_extent_deg=.6,shore_distance_shared=True,ocean_displacement_max_cm=36,star_field='Procedural / exposure-dependent',cloud_start_max_distance_km=1800)
(SAVED_ROOT/'world-continuity-assets.json').write_text(json.dumps(report,indent=2))
print('WORLD_CONTINUITY_ASSETS_READY')

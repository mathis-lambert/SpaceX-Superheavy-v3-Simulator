"""Author one geographic shading contract across all Earth geometry scales.

Run after the original Earth build. Geometry coverage no longer controls imagery
coverage: all shells use the same geodetic UVs, water BRDF and fallback blending.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from project_paths import ART_ROOT, SAVED_ROOT
from unreal_materials import u, E, A, prop, expression as ex, material, custom, sample, scalar, vector, constant, color, connect, save
import json
import math

ROOT='/Game/Starbase'
tools=u.AssetToolsHelpers.get_asset_tools()
def import_texture(name,path):
    existing=u.load_asset(ROOT+'/Textures/Earth/'+name)
    if existing and '-WorldReimportTextures' not in u.SystemLibrary.get_command_line():return existing
    task=u.AssetImportTask();task.filename=str(path);task.destination_path=ROOT+'/Textures/Earth';task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    tools.import_asset_tasks([task]);t=u.load_asset(task.destination_path+'/'+name);assert t,name
    prop(t,'srgb',True);prop(t,'compression_settings',u.TextureCompressionSettings.TC_BC7)
    prop(t,'address_x',u.TextureAddress.TA_CLAMP);prop(t,'address_y',u.TextureAddress.TA_CLAMP)
    prop(t,'never_stream',False);prop(t,'lod_bias',0);prop(t,'max_texture_size',8192)
    prop(t,'mip_gen_settings',u.TextureMipGenSettings.TMGS_SHARPEN1);assert A.save_loaded_asset(t,False)
    return t

coast=import_texture('T_CoastContinuous',ART_ROOT/'Earth/Continuity/Coast8192.png')
gulf=import_texture('T_GulfContinuous',ART_ROOT/'Earth/Continuity/Gulf8192.png')
globe=u.load_asset(ROOT+'/Textures/Earth/T_EarthSeptember')
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
    water=custom(m,{'C':base},'return smoothstep(.001,.012,C.b-C.r*1.13)*smoothstep(0,.009,C.g-C.r*.9);',u.CustomMaterialOutputType.CMOT_FLOAT1)
    if tile is not None:
        i,j=tile;half_lat=math.degrees(6000/6371000);half_lon=half_lat/math.cos(math.radians(25.9973))
        lo=-97.1569-half_lon+i*half_lon/2;hi=25.9973-half_lat+(j+1)*half_lat/2
        tileuv=custom(m,{'G':geo},f'return float2((G.x-({lo:.12f}))/{half_lon/2:.12f},({hi:.12f}-G.y)/{half_lat/2:.12f});',u.CustomMaterialOutputType.CMOT_FLOAT2)
        aerial=sample(m,ROOT+f'/Textures/Earth/T_BocaChica_{i}_{j}',tileuv)
        water=custom(m,{'W':water,'P':p},'float h=(P.z+dot(P.xy,P.xy)/1274200000.)*.01;float local=1-smoothstep(-4.45,-3.85,h);return lerp(W,local,smoothstep(0,60000,600000-max(abs(P.x),abs(P.y))));',u.CustomMaterialOutputType.CMOT_FLOAT1)
        base=custom(m,{'B':base,'C':(aerial,'RGB'),'A':(aerial,'A'),'UV':tileuv,'P':p,'Camera':camera},'''
float edge=600000-max(abs(P.x),abs(P.y));
float near=1-smoothstep(180000,1000000,length(Camera-P));
return lerp(B,C,A*smoothstep(0,60000,edge)*near);''')
    # Water comes from a BRDF, not a lit photograph of an old ocean surface.
    base=custom(m,{'C':base,'W':water},'float3 sea=lerp(float3(.003,.013,.024),float3(.009,.035,.041),saturate(C.g*4));return lerp(C,sea,W);')
    grounduv=custom(m,{'P':p},'return P.xy/430;',u.CustomMaterialOutputType.CMOT_FLOAT2)
    grain=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_col',grounduv)
    ground=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_nrm',grounduv,normal=True)
    base=custom(m,{'C':base,'D':(grain,'RGB'),'W':water,'P':p,'Camera':camera},'float detail=(1-W)*(1-smoothstep(15000,150000,length(Camera-P)));return C*lerp(1.,.8+dot(D,float3(.299,.587,.114))*.45,detail*.45);')
    connect(m,base,u.MaterialProperty.MP_BASE_COLOR)
    uv1=custom(m,{'P':p,'T':time},'return P.xy/2400+float2(T*.009,T*.003);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    uv2=custom(m,{'P':p,'T':time},'return P.yx/7300+float2(-T*.004,T*.002);',u.CustomMaterialOutputType.CMOT_FLOAT2)
    wavepath='/Game/ThirdParty/WaterMaterials/Textures/T_Ocean_Waves01_Normals'
    w1=sample(m,wavepath,uv1,sampler=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    w2=sample(m,wavepath,uv2,sampler=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    vertex=ex(m,u.MaterialExpressionVertexNormalWS)
    normal=custom(m,{'P':p,'Camera':camera,'W':water,'A':(w1,'RGB'),'B':(w2,'RGB'),'Terrain':vertex,'Ground':(ground,'RGB')},'''
float3 up=normalize(P+float3(0,0,637100000));
float3 east=normalize(cross(float3(0,1,0),up));float3 north=cross(up,east);
float detail=1-smoothstep(250000,2000000,length(Camera-P));
float2 slope=((A.xy*2-1)*.34+(B.xy*2-1)*.2)*detail;
float near=1-smoothstep(15000,150000,length(Camera-P));
float3 land=normalize(Terrain+(east*Ground.x+north*Ground.y)*near*.24);
return normalize(lerp(land,normalize(up+east*slope.x+north*slope.y),W));''')
    connect(m,normal,u.MaterialProperty.MP_NORMAL)
    rough=custom(m,{'W':water,'P':p,'Camera':camera},'return lerp(.86,lerp(.14,.28,smoothstep(100000,1500000,length(Camera-P))),W);',u.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(m,rough,u.MaterialProperty.MP_ROUGHNESS)
    connect(m,constant(m,.38),u.MaterialProperty.MP_SPECULAR)
    save(m)
    return m

for name in ('M_EarthGlobe','M_GulfRegion','M_BocaRegion'):build_surface(name)
for j in range(4):
    for i in range(4):build_surface(f'M_BocaChica_{i}_{j}',(i,j))

# Low-light star field is scene-linear emissive, so normal daylight exposure
# naturally suppresses it. Procedural distribution is not an astronomy catalogue.
stars=material(ROOT+'/Materials/M_StarField');prop(stars,'shading_model',u.MaterialShadingModel.MSM_UNLIT)
prop(stars,'two_sided',True);prop(stars,'is_sky',True)
p=ex(stars,u.MaterialExpressionWorldPosition);c=ex(stars,u.MaterialExpressionCameraPositionWS)
sky=ex(stars,u.MaterialExpressionSkyAtmosphereViewLuminance)
emission=custom(stars,{'P':p,'C':c,'Sky':sky},'''
float3 d=normalize(P-C);float2 uv=float2(atan2(d.y,d.x)/6.2831853+.5,acos(clamp(d.z,-1,1))/3.14159265);
float2 grid=uv*float2(1440,720),cell=floor(grid),f=frac(grid);
float3 h=frac(cell.xyx*float3(.1031,.1030,.0973));h+=dot(h,h.yxz+33.33);h=frac((h.xxy+h.yxx)*h.zyx);
float r=length(f-(.2+h.xy*.6));float aa=max(length(fwidth(grid)),.025);
float star=(1-smoothstep(.07,.07+aa*.6,r))*step(.996,h.z);
return Sky+MaterialExpressionSkyAtmosphereLightDiskLuminance(Parameters,0,-1)+MaterialExpressionSkyAtmosphereLightDiskLuminance(Parameters,1,-1)+star*lerp(float3(.65,.8,1),float3(1,.8,.58),h.x)*220;''')
connect(stars,emission,u.MaterialProperty.MP_EMISSIVE_COLOR);save(stars)

levels=u.get_editor_subsystem(u.LevelEditorSubsystem);assert levels.load_level(ROOT+'/Maps/L_RecoveryLab')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='Recovery_Clouds':
        cloud=a.get_component_by_class(u.VolumetricCloudComponent)
        prop(cloud,'tracing_start_max_distance',1800.)
        prop(cloud,'tracing_max_distance_mode',u.VolumetricCloudTracingMaxDistanceMode.DISTANCE_FROM_CLOUD_LAYER_ENTRY_POINT)
        prop(cloud,'tracing_max_distance',450.)
    if a.get_actor_label()=='Recovery_Sun':
        prop(a.light_component,'cloud_shadow_extent',150.)
levels.save_current_level()
report=dict(success=True,materials=19,shared_geographic_contract=True,coast_pixels=8192,gulf_pixels=8192,star_field='Procedural / exposure-dependent',cloud_start_max_distance_km=1800)
(SAVED_ROOT/'world-continuity-assets.json').write_text(json.dumps(report,indent=2))
print('WORLD_CONTINUITY_ASSETS_READY')

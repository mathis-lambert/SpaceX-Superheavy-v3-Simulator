"""Author bounded-cost spherical weather: coverage skipping and distance LOD."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,prop,material,expression,scalar,vector,custom,color,connect,save,E
from project_paths import ART_ROOT

assets=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
folder='/Game/Starbase/Textures/Weather'
def import_texture(filename,name):
    task=u.AssetImportTask();task.filename=str(ART_ROOT/'Weather'/filename)
    task.destination_path=folder;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    tools.import_asset_tasks([task]);tex=u.load_asset(folder+'/'+name);assert tex
    prop(tex,'srgb',False);prop(tex,'compression_settings',u.TextureCompressionSettings.TC_MASKS)
    assets.save_loaded_asset(tex,False);return tex
atlas=import_texture('CloudShape128.png','T_CloudShapeAtlas')
weather=import_texture('CloudWeather2048.png','T_CloudWeather')
prop(weather,'address_y',u.TextureAddress.TA_CLAMP);assets.save_loaded_asset(weather,False)
volume=u.load_asset(folder+'/T_CloudShape128') or tools.create_asset('T_CloudShape128',folder,u.VolumeTexture,u.VolumeTextureFactory())
prop(volume,'source2d_texture',atlas);prop(volume,'source2d_tile_size_x',128);prop(volume,'source2d_tile_size_y',128)
prop(volume,'srgb',False);prop(volume,'compression_settings',u.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
prop(volume,'filter',u.TextureFilter.TF_TRILINEAR)
assets.save_loaded_asset(volume,False)

m=material('/Game/Starbase/Materials/M_LayeredWeather')
prop(m,'blend_mode',u.BlendMode.BLEND_ADDITIVE);prop(m,'material_domain',u.MaterialDomain.MD_VOLUME)
prop(m,'used_with_volumetric_cloud',True)
p=expression(m,u.MaterialExpressionWorldPosition)
camera=expression(m,u.MaterialExpressionCameraPositionWS)
time=expression(m,u.MaterialExpressionTime)
wind=vector(m,'WindMps',(8,3,0,0));coverage=scalar(m,'Coverage',.48)
def texture_object(texture):
    node=expression(m,u.MaterialExpressionTextureObject);prop(node,'texture',texture)
    prop(node,'sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if texture==volume else u.MaterialSamplerType.SAMPLERTYPE_MASKS);return node
weather_obj=texture_object(weather);shape_obj=texture_object(volume)
header='''
float3 metres=P*.01;
float3 radial=metres+float3(0,0,6371000);
float h=length(radial)-6371000;
float3 n=normalize(radial-W.xyz*T);
n=float3(.5*n.x+.8660254*n.z,n.y,.8660254*n.x-.5*n.z);
float2 weatherUV=float2(atan2(n.y,n.x)*.159154943+.5,acos(clamp(n.z,-1,1))*.318309886);
float3 weather=Texture2DSampleLevel(Weather,WeatherSampler,weatherUV,0).rgb;
// Keep fair-weather cumulus between large systems; Overcast really closes the
// regional gaps, rather than merely lowering the weather-map threshold.
float regionalCover=smoothstep(.35,.7,weather.r);
float cover=(.18+.75*regionalCover)*smoothstep(.08,.48,C);
cover=lerp(cover,1,smoothstep(.65,.84,C));
float highCover=smoothstep(.57,.75,weather.b)*High;
'''
inputs={'P':p,'Camera':camera,'T':time,'W':wind,'C':coverage,'High':scalar(m,'HighClouds',.65),'Weather':weather_obj,'Shape':shape_obj}
density=custom(m,inputs,header+'''
float lowProfile=smoothstep(700,1050,h)*(1-smoothstep(1700,2900,h));
float midProfile=smoothstep(3900,4400,h)*(1-smoothstep(5400,6400,h));
float highProfile=smoothstep(8300,8500,h)*(1-smoothstep(9100,9500,h));
if(cover<.001 && highCover*highProfile<.001)return 0;
float distanceM=length(P-Camera)*.01;
float mip=clamp(log2(max(distanceM,80000)/80000),0,5);
// A low-frequency, independently rotated field breaks the small noise tile's
// visible periodicity and groups cells into banks, with clear lanes between.
float3 advected=metres-W.xyz*T;
// Wind shear and anisotropy distinguish middle-level banks and high streaks
// without another volume lookup, extra layer component or altitude switch.
float upper=smoothstep(3000,7000,h);
advected.xy-=W.xy*T*upper*.65+float2(7300,-11300)*upper;
float3 macroUV=float3(advected.x*.8+advected.y*.6,advected.y*.8-advected.x*.6,advected.z)/173000;
float3 macro=Texture3DSampleLevel(Shape,ShapeSampler,macroUV+float3(.31,.71,.43),0).rgb;
float bank=smoothstep(.26,.64,macro.r);
cover*=lerp(.55+.45*bank,1,smoothstep(.65,.84,C));
float3 uv=advected/6000+(macro-.5)*5;
uv.xy=lerp(uv.xy,float2(uv.x*.18+uv.y*.08,uv.y*1.8-uv.x*.35),highProfile);
float base=Texture3DSampleLevel(Shape,ShapeSampler,uv,mip).r;
float detail=0;
// Branch inside the custom node: no distant erosion texture fetch.
if(distanceM<50000 && cover>.001)
    detail=(1-Texture3DSampleLevel(Shape,ShapeSampler,uv*5.7+float3(.17,.53,.29),mip).g)
        *.09*(1-smoothstep(12000,50000,distanceM));
float low=saturate((base-(.80-.55*cover)-(1-lowProfile)*.55-detail)*7)*lowProfile;
float mid=saturate((base-(.85-.55*cover)-(1-midProfile)*.6-detail)*7)*midProfile;
float high=highCover*highProfile*(.65+.35*base);
return (low*.12+mid*.06*lerp(.25,1,weather.g)+high*.001)*.01;
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m,density,u.MaterialProperty.MP_SUBSURFACE_COLOR)
connect(m,color(m,(.98,.985,1)),u.MaterialProperty.MP_BASE_COLOR)
connect(m,color(m,(0,0,0)),u.MaterialProperty.MP_EMISSIVE_COLOR)
advanced=expression(m,u.MaterialExpressionVolumetricAdvancedMaterialOutput)
prop(advanced,'multi_scattering_approximation_octave_count',1)
prop(advanced,'ground_contribution',False);prop(advanced,'const_phase_g',.65)
prop(advanced,'gray_scale_material',True)
occupied=custom(m,{k:v for k,v in inputs.items() if k not in ('Shape','Camera')},header+'''
bool low=(h>690 && h<2910)||(h>3890 && h<6410);
bool high=h>8290 && h<9510;
return float3((low && cover>.001)||(high && highCover>.001),0,0);
''')
pin=next(name for name in E.get_material_expression_input_names(advanced) if 'Conservative' in str(name))
assert E.connect_material_expressions(occupied,'',advanced,pin)
save(m)
# The level and runtime reference the same material, including startup/PSO warmup.
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    sky=actor.get_component_by_class(u.SkyLightComponent)
    if sky:
        prop(sky,'mobility',u.ComponentMobility.MOVABLE)
        prop(sky,'real_time_capture',True)
    cloud=actor.get_component_by_class(u.VolumetricCloudComponent)
    if cloud:
        prop(cloud,'material',m)
        prop(cloud,'layer_bottom_altitude',.6);prop(cloud,'layer_height',10.4)
        prop(cloud,'use_per_sample_atmospheric_light_transmittance',True)
        prop(cloud,'shadow_tracing_distance',4.)
assert levels.save_current_level()
print('LAYERED_WEATHER_READY / weather skipping / distance-filtered cloud noise',flush=True)

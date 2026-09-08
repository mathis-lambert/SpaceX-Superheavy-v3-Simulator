"""Author the generated Recovery smoke and propulsion materials in Unreal.

Run after bake_vapor_atlas.py. Safe to rerun: only generated VFX assets change.
The Niagara stack authoring recipe is recorded separately in recovery_niagara.json.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
from unreal_materials import expression as expr, scalar, custom, save, connect

import unreal as u
from pathlib import Path
A=u.EditorAssetLibrary; E=u.MaterialEditingLibrary; AT=u.AssetToolsHelpers.get_asset_tools()
ROOT='/Game/Starbase'

def material(name,blend):
    path=ROOT+'/Materials/'+name
    m=u.load_asset(path) if A.does_asset_exist(path) else AT.create_asset(name,ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
    E.delete_all_material_expressions(m)
    m.set_editor_property('blend_mode',blend);m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT);m.set_editor_property('two_sided',True)
    return m

source=ART_ROOT/'Flight'/'T_RecoveryVaporAtlas.png'
assert source.exists(),str(source)
task=u.AssetImportTask();task.filename=str(source);task.destination_path=ROOT+'/Textures';task.destination_name='T_RecoveryVaporAtlas';task.automated=True;task.save=True;task.replace_existing=True;AT.import_asset_tasks([task])
atlas=u.load_asset(ROOT+'/Textures/T_RecoveryVaporAtlas')
atlas.set_editor_property('srgb',False);atlas.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_BC7)
atlas.set_editor_property('address_x',u.TextureAddress.TA_CLAMP);atlas.set_editor_property('address_y',u.TextureAddress.TA_CLAMP);A.save_loaded_asset(atlas,False)

for name,niagara in [('M_GroundVapor',False),('M_VaporTrail',True)]:
    m=material(name,u.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('used_with_niagara_sprites',True)
    uv=expr(m,u.MaterialExpressionTextureCoordinate)
    if niagara:
        seed=expr(m,u.MaterialExpressionParticleRandom);age=expr(m,u.MaterialExpressionParticleRelativeTime)
        opacity=expr(m,u.MaterialExpressionParticleColor)
        fade=custom(m,{'Age':age,'Alpha':(opacity,'A')},'return Alpha*smoothstep(0,0.018,Age)*pow(saturate(1-Age),1.1)*0.42;',u.CustomMaterialOutputType.CMOT_FLOAT1)
    else:
        seed=scalar(m,'Seed',0);age=scalar(m,'Age',0);fade=scalar(m,'Opacity',0.6)
    atlas_uv=custom(m,{'UV':uv,'Seed':seed,'Age':age},'float id=floor(frac(Seed*0.6180339)*16);float2 q=UV+0.018*sin(UV.yx*18+float2(Seed*13+Age*0.5,Seed*5-Age*0.35))*sin(UV*3.14159);q=clamp(q,0.008,0.992);return (q+float2(fmod(id,4),floor(id/4)))/4;',u.CustomMaterialOutputType.CMOT_FLOAT2)
    tex=expr(m,u.MaterialExpressionTextureSample);tex.set_editor_property('texture',atlas);tex.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    pins=E.get_material_expression_input_names(tex)
    assert pins,'Texture sample has no inputs'
    assert E.connect_material_expressions(atlas_uv,'',tex,pins[0]),str(pins)
    tint=expr(m,u.MaterialExpressionVectorParameter);tint.set_editor_property('parameter_name','Tint');tint.set_editor_property('default_value',u.LinearColor(.88,.94,1,1))
    rgb=custom(m,{'Shade':tex,'Tint':tint},'return Tint*(2100+10500*Shade.r);')
    connect(m, rgb, u.MaterialProperty.MP_EMISSIVE_COLOR)
    density=custom(m,{'Alpha':(tex,'A'),'Fade':fade},'return saturate(Alpha*Fade);',u.CustomMaterialOutputType.CMOT_FLOAT1)
    depth=expr(m,u.MaterialExpressionDepthFade);depth.set_editor_property('fade_distance_default',180)
    assert E.connect_material_expressions(density,'',depth,'Opacity')
    connect(m, depth, u.MaterialProperty.MP_OPACITY);save(m)

flame=material('M_RaptorPlume',u.BlendMode.BLEND_ADDITIVE)
uv=expr(flame,u.MaterialExpressionTextureCoordinate);t=scalar(flame,'FlightTime',0)
th=scalar(flame,'Throttle',1);vac=scalar(flame,'Vacuum',0);mix=scalar(flame,'MixingLayer',0)
NOISE='''struct FlameNoise {
 float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
 float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
 float f(float2 p){return .57*n(p)+.28*n(p*2.07+9)+.15*n(p*4.13+31);}
}; FlameNoise N;
'''
inputs={'UV':uv,'T':t,'Throttle':th,'Vacuum':vac,'Mix':mix}
rgb=custom(flame,inputs,NOISE+'''float z=UV.y;
float n=N.f(float2(UV.x*9,z*16-T*9));
float diamond=pow(saturate(sin(z*(68-30*Vacuum)-0.4)),18)*(1-Vacuum)*(1-Mix);
float3 core=lerp(float3(.18,.34,1),float3(1,.25,.035),smoothstep(.24,.82,z));
float3 col=lerp(core,float3(1,.30,.045),Mix);col=lerp(col,float3(.85,.85,1),diamond*.7);
return col*(lerp(2600,11500,Mix)+diamond*12000)*(0.45+n)*Throttle;
''')
connect(flame, rgb, u.MaterialProperty.MP_EMISSIVE_COLOR)
opacity=custom(flame,inputs,NOISE+'''float z=UV.y;float n=N.f(float2(UV.x*12,z*21-T*10));
float edge=smoothstep(0,.025,z)*pow(saturate(1-z),1.6);
float breakup=smoothstep(lerp(.12,.48,z),.75,n);
return edge*breakup*lerp(.48,.24,Mix)*(1-.65*Vacuum*Mix);
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(flame, opacity, u.MaterialProperty.MP_OPACITY)
# Small world-space displacement erodes the mesh silhouette without moving the nozzle.
normal=expr(flame,u.MaterialExpressionVertexNormalWS)
wpo=custom(flame,{**inputs,'Normal':normal},NOISE+'float z=UV.y;float n=N.f(float2(UV.x*8,z*13-T*8));return Normal*(n-.45)*sin(z*3.14159)*lerp(30,160,Mix);')
connect(flame, wpo, u.MaterialProperty.MP_WORLD_POSITION_OFFSET);save(flame)
print('RECOVERY_VFX_READY')
rock_path=ROOT+'/Materials/M_RecoveryRock'
rock=u.load_asset(rock_path) if A.does_asset_exist(rock_path) else A.duplicate_asset('/Game/ThirdParty/WaterMaterials/Materials/M_River_Rock',rock_path)
rock.set_editor_property('used_with_instanced_static_meshes',True);save(rock)
if '-run=pythonscript' not in u.SystemLibrary.get_command_line().lower():
    system=u.load_asset(ROOT+'/FX/NS_RecoveryVaporTrail')
    if system:u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([system])

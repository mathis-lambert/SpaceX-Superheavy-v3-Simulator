"""Import supporting artwork and update lighting/materials without rebuilding Earth."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
from unreal_materials import prop, expression as ex, scalar, constant as const, custom, connect, sample, save

import unreal as u,math
from pathlib import Path
ROOT='/Game/Starbase';SRC=ART_ROOT/'VehicleDetails'
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary;AT=u.AssetToolsHelpers.get_asset_tools()
def mat(name):
    path=ROOT+'/Materials/'+name;m=u.load_asset(path) if A.does_asset_exist(path) else AT.create_asset(name,ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
    E.delete_all_material_expressions(m);prop(m,'used_with_nanite',True);return m
steel=mat('M_StainlessFlight');uv=ex(steel,u.MaterialExpressionTextureCoordinate)
color=custom(steel,{'UV':uv},'''float weld=pow(abs(cos(UV.y*54/1.82*3.14159)),90);float brush=sin(UV.x*1700)*.015;return float3(.56,.59,.62)*(1-weld*.22+brush);''')
connect(steel,color,u.MaterialProperty.MP_BASE_COLOR);connect(steel,const(steel,1),u.MaterialProperty.MP_METALLIC)
rough=custom(steel,{'UV':uv},'return .24+.06*sin(UV.y*163)*sin(UV.x*49);',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(steel,rough,u.MaterialProperty.MP_ROUGHNESS);save(steel)
tile=mat('M_StarshipHeatShield');uv=ex(tile,u.MaterialExpressionTextureCoordinate)
hexcode='''float2 p=UV*float2(160,300);p.x+=fmod(floor(p.y),2)*.5;float2 q=abs(frac(p)-.5);float d=max(q.x*.866+q.y*.5,q.y);float edge=1-smoothstep(.445-fwidth(d),.465+fwidth(d),d);float n=frac(sin(dot(floor(p),float2(12.9898,78.233)))*43758.5453);return float3(.024,.027,.031)*lerp(.32,.80+n*.35,edge);'''
connect(tile,custom(tile,{'UV':uv},hexcode),u.MaterialProperty.MP_BASE_COLOR);connect(tile,const(tile,.88),u.MaterialProperty.MP_ROUGHNESS);save(tile)
dark=mat('M_FlightMechanisms');connect(dark,custom(dark,{},'return float3(.065,.075,.085);'),u.MaterialProperty.MP_BASE_COLOR);connect(dark,const(dark,.8),u.MaterialProperty.MP_METALLIC);connect(dark,const(dark,.46),u.MaterialProperty.MP_ROUGHNESS);save(dark)
nozzle=mat('M_FlightNozzles');connect(nozzle,custom(nozzle,{},'return float3(.23,.26,.29);'),u.MaterialProperty.MP_BASE_COLOR);connect(nozzle,const(nozzle,.88),u.MaterialProperty.MP_METALLIC);connect(nozzle,const(nozzle,.33),u.MaterialProperty.MP_ROUGHNESS);save(nozzle)
mapping={'Steel':steel,'Tiles':tile,'Mechanisms':dark,'Nozzles':nozzle}
for name in ['SM_StarshipDetailed','SM_RCSBlock']:
    task=u.AssetImportTask();task.filename=str(SRC/(name+'.fbx'));task.destination_path=ROOT+'/Meshes';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=False
    opts=u.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False;prop(opts,'automated_import_should_detect_type',False);opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
    opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;task.options=opts
    AT.import_asset_tasks([task]);m=u.load_asset(ROOT+'/Meshes/'+name);assert m
    for i,slot in enumerate(m.get_editor_property('static_materials')):
        label=str(slot.material_slot_name);chosen=next((v for k,v in mapping.items() if k in label),steel);m.set_material(i,chosen)
        print('ART_SLOT',name,i,label,chosen.get_name())
    settings=m.get_editor_property('nanite_settings');prop(settings,'enabled',True);prop(settings,'position_precision',2);prop(m,'nanite_settings',settings);A.save_loaded_asset(m,False)
gas=mat('M_AttitudeGas');prop(gas,'blend_mode',u.BlendMode.BLEND_ADDITIVE);prop(gas,'shading_model',u.MaterialShadingModel.MSM_UNLIT);prop(gas,'two_sided',True)
uv=ex(gas,u.MaterialExpressionTextureCoordinate);power=scalar(gas,'Power',0);time=scalar(gas,'Time',0)
connect(gas,custom(gas,{'UV':uv,'P':power,'T':time},'return float3(.46,.64,1)*2300*P*pow(saturate(1-UV.y),2)*(0.7+0.3*sin(UV.y*47-T*33));'),u.MaterialProperty.MP_EMISSIVE_COLOR)
connect(gas,custom(gas,{'UV':uv,'P':power},'return .25*P*smoothstep(0,.04,UV.y)*pow(saturate(1-UV.y),1.4);',u.CustomMaterialOutputType.CMOT_FLOAT1),u.MaterialProperty.MP_OPACITY);save(gas)
# Preserve the existing authored atlas, but let real scene light illuminate it.
for name in ['M_GroundVapor','M_VaporTrail']:
    m=u.load_asset(ROOT+'/Materials/'+name);prop(m,'shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    prop(m,'translucency_lighting_mode',u.TranslucencyLightingMode.TLM_VOLUMETRIC_DIRECTIONAL)
    nodes=E.get_material_expressions(m)
    shades=[n for n in nodes if isinstance(n,u.MaterialExpressionTextureSample)]
    tints=[n for n in nodes if isinstance(n,u.MaterialExpressionVectorParameter) and str(n.get_editor_property('parameter_name'))=='Tint']
    assert shades and tints,name
    rgb=custom(m,{'Shade':(shades[0],'RGB'),'Tint':tints[0]},'return Tint*(.38+.62*Shade.r);')
    connect(m,rgb,u.MaterialProperty.MP_BASE_COLOR);connect(m,const(m,0),u.MaterialProperty.MP_EMISSIVE_COLOR)
    connect(m,const(m,1),u.MaterialProperty.MP_ROUGHNESS);save(m)
# Warm, nearly white luminous exhaust merging into a turbulent orange edge.
m=u.load_asset(ROOT+'/Materials/M_RaptorPlume')
for node in E.get_material_expressions(m):
    if isinstance(node,u.MaterialExpressionCustom):
        code=node.get_editor_property('code')
        if 'float3 core=' in code:
            code=code.replace('float3(.18,.34,1),float3(1,.25,.035)','float3(.48,.62,1),float3(1,.42,.12)')
            code=code.replace('float3(1,.30,.045)','float3(1,.62,.32)').replace('lerp(2600,11500,Mix)','lerp(28000,38000,Mix)').replace('diamond*12000','diamond*35000')
            prop(node,'code',code)
save(m)
# Higher detail textures replace the same registered geographic assets.
for name,file,size in [(f'T_BocaChica_{i}_{j}',f'BocaChica_{i}_{j}.png',4096) for j in range(4) for i in range(4)]+[('T_BocaRegion','BocaRegionHi.png',8192)]:
    assert (SRC/file).exists(),file
    t=u.AssetImportTask();t.filename=str(SRC/file);t.destination_path=ROOT+'/Textures/Earth';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=False;AT.import_asset_tasks([t])
    asset=u.load_asset(ROOT+'/Textures/Earth/'+name);prop(asset,'power_of_two_mode',u.TexturePowerOfTwoSetting.RESIZE_TO_SPECIFIC_RESOLUTION)
    prop(asset,'resize_during_build_x',size);prop(asset,'resize_during_build_y',size);prop(asset,'max_texture_size',size)
    prop(asset,'compression_settings',u.TextureCompressionSettings.TC_BC7);prop(asset,'mip_gen_settings',u.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    prop(asset,'address_x',u.TextureAddress.TA_CLAMP);prop(asset,'address_y',u.TextureAddress.TA_CLAMP);A.save_loaded_asset(asset,False)
earth=u.load_asset(ROOT+'/Textures/Earth/T_EarthSeptember')
prop(earth,'resize_during_build_x',16384);prop(earth,'resize_during_build_y',8192);prop(earth,'max_texture_size',16384);A.save_loaded_asset(earth,False)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);levels.load_level(ROOT+'/Maps/L_RecoveryLab');ed=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in ed.get_all_level_actors():
    label=a.get_actor_label()
    if label=='Recovery_Sun':
        c=a.light_component;c.set_mobility(u.ComponentMobility.MOVABLE);prop(c,'light_source_angle',.5357);prop(c,'shadow_bias',.3);prop(c,'shadow_slope_bias',.4);prop(c,'contact_shadow_length',.05);prop(c,'cloud_shadow_strength',.55)
    if label=='Recovery_Clouds':
        c=a.get_component_by_class(u.VolumetricCloudComponent);prop(c,'view_sample_count_scale',2.);prop(c,'shadow_view_sample_count_scale',2.);prop(c,'layer_bottom_altitude',2.1);prop(c,'layer_height',3.1)
        m=c.get_editor_property('material');print('CLOUD_SCALARS',E.get_scalar_parameter_names(m));print('CLOUD_VECTORS',E.get_vector_parameter_names(m))
    if label=='Recovery_Fog':
        c=a.component;prop(c,'enable_volumetric_fog',True);prop(c,'fog_height_falloff',.22);prop(c,'volumetric_fog_distance',150000);prop(c,'volumetric_fog_scattering_distribution',.65)
    if label=='Recovery_LaunchMount':a.set_actor_location(u.Vector(2400,0,0),False,False)
    if label.startswith('Recovery_Launch') and label!='Recovery_LaunchMount':
        p=a.get_actor_location()
        if p.x>7000:p.x-=11600;a.set_actor_location(p,False,False)
    if label=='Recovery_Tower':a.set_editor_property('arm_contact_height_above_base_m',61.5678)
levels.save_current_level();print('OVERHAUL_PRESENTATION_READY')

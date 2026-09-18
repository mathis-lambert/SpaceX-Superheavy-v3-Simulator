"""Author light-reactive 3D vapor, using two bounded volume-texture samples."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from unreal_materials import u, material, prop, expression, scalar, vector, color, custom, sample, connect, save
from project_paths import ART_ROOT

# Import an authored periodic volume with known per-channel density statistics.
tools=u.AssetToolsHelpers.get_asset_tools();assets=u.EditorAssetLibrary
task=u.AssetImportTask();task.filename=str(ART_ROOT/'Effects/FlowNoise/FlowNoise128.png')
task.destination_path='/Game/Starbase/Textures/Effects';task.destination_name='T_FlowNoiseAtlas'
task.automated=True;task.replace_existing=True;task.save=True;tools.import_asset_tasks([task])
atlas=u.load_asset(task.destination_path+'/'+task.destination_name);assert atlas
prop(atlas,'srgb',False);prop(atlas,'compression_settings',u.TextureCompressionSettings.TC_MASKS);assets.save_loaded_asset(atlas,False)
volume_path='/Game/Starbase/Textures/Effects/T_FlowNoise128'
volume=u.load_asset(volume_path)
if not volume:
    factory=u.VolumeTextureFactory()
    volume=tools.create_asset('T_FlowNoise128','/Game/Starbase/Textures/Effects',u.VolumeTexture,factory)
assert volume
prop(volume,'source2d_texture',atlas);prop(volume,'source2d_tile_size_x',128);prop(volume,'source2d_tile_size_y',128)
prop(volume,'srgb',False);prop(volume,'compression_settings',u.TextureCompressionSettings.TC_MASKS)
assets.save_loaded_asset(volume,False)

m = material('/Game/Starbase/Materials/M_VolumetricVapor')
prop(m, 'blend_mode', u.BlendMode.BLEND_ADDITIVE)
prop(m, 'material_domain', u.MaterialDomain.MD_VOLUME)
position = expression(m, u.MaterialExpressionWorldPosition)
center = expression(m, u.MaterialExpressionObjectPositionWS)
radius = scalar(m, 'RadiusCm', 1000)
age = scalar(m, 'Age', 0)
density = scalar(m, 'Density', 0)
axis = vector(m, 'FlowAxis', (1, 0, 0, 1))
scale = vector(m, 'ShapeScale', (1, 1, 1, 1))
local = custom(m, {'P': position, 'C': center, 'R': radius, 'Axis': axis, 'Scale': scale}, '''
float3 x = normalize(Axis.xyz);
float3 y = normalize(cross(abs(x.z)<.95 ? float3(0,0,1) : float3(0,1,0), x));
float3 z = cross(x,y);
float3 w = P-C;
return float3(dot(w,x),dot(w,y),dot(w,z))/max(R*Scale.xyz,1);
''')
seed=scalar(m,'Seed',0)
uv = custom(m, {'Q': local, 'T': age, 'S':seed}, 'return Q*.43 + float3(S,S*.37,S*.73) + float3(.018,-.012,-.023)*T;')
noise = sample(m, volume, uv, sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
fine_uv = custom(m, {'UV': uv}, 'return UV*3.13+float3(.27,.14,.53);')
fine = sample(m, volume, fine_uv, sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
for texture_node in (noise, fine):
    # Fog voxelization has no reliable screen derivatives for implicit 3D mip selection.
    prop(texture_node, 'mip_value_mode', u.TextureMipValueMode.TMVM_MIP_LEVEL)
    prop(texture_node, 'const_mip_value', 0)
extinction = custom(m, {'Q': local, 'N': noise, 'F': fine, 'D': density}, '''
float3 warp=(N.gbr-.5)*.70+(F.brg-.5)*.20;
float envelope=saturate((1-length(Q+warp))/.30);
// Domain warping must not push visible density against the voxel bounds.
float boundary=1-smoothstep(.78,.99,max(abs(Q.x),max(abs(Q.y),abs(Q.z))));
float field=N.r*.62+N.g*.25+F.b*.13;
float erosion=(1-envelope)*.27;
float shape=smoothstep(.33,.63,field-erosion);
// The fog grid integrates centimetres. A .105 coefficient made a metre of
// vapour effectively opaque and hid all internal erosion as a white blob.
// This transported mist complements the dense near-source sparse volume;
// it must not cover that resolved flow with another opaque spherical mass.
return D*.0006*shape*envelope*boundary;
''', u.CustomMaterialOutputType.CMOT_FLOAT1)
# In UE 5.8 RGB extinction occupies Subsurface Color; Opacity is unused for volumes.
prop(extinction, 'description', 'Vapor extinction / inverse centimetres')
connect(m, extinction, u.MaterialProperty.MP_SUBSURFACE_COLOR)
connect(m, color(m, (.82,.85,.89)), u.MaterialProperty.MP_BASE_COLOR)
connect(m, color(m, (0,0,0)), u.MaterialProperty.MP_EMISSIVE_COLOR)
save(m)
print('VOLUMETRIC_VAPOR_READY', m.get_path_name())

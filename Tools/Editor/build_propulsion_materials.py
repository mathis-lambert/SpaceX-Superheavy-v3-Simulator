"""One authoritative pressure/thrust-driven flame, with bounded texture cost."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from unreal_materials import u, prop, material, expression, scalar, custom, sample, connect, save

m = material('/Game/Starbase/Materials/M_RaptorPlume')
prop(m, 'blend_mode', u.BlendMode.BLEND_ADDITIVE)
prop(m, 'shading_model', u.MaterialShadingModel.MSM_UNLIT)
prop(m, 'two_sided', True)
uv = expression(m, u.MaterialExpressionTextureCoordinate)
t = scalar(m, 'FlightTime', 0)
throttle = scalar(m, 'Throttle', 0)
vacuum = scalar(m, 'Vacuum', 0)
mix = scalar(m, 'MixingLayer', 0)
# Periodic cylindrical coordinates join at the UV seam. One explicit-LOD
# volume sample replaces repeated multi-octave procedural hash evaluations.
flow_uv = custom(m, {'UV': uv, 'T': t}, '''
float a=UV.x*6.2831853;
return float3(sin(a)*.28,cos(a)*.28,UV.y*2.7-T*1.3);
''')
noise = sample(m, '/Game/Starbase/Textures/Effects/T_FlowNoise128', flow_uv,
               sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
prop(noise, 'mip_value_mode', u.TextureMipValueMode.TMVM_MIP_LEVEL)
prop(noise, 'const_mip_value', 1)
inputs = {'UV': uv, 'N': noise, 'Throttle': throttle, 'Vacuum': vacuum, 'Mix': mix}
emission = custom(m, inputs, '''
float z=UV.y;
float spacing=lerp(54,78,sqrt(saturate(Throttle)))-28*Vacuum;
float phase=z*spacing-.4;
float diamond=pow(saturate(sin(phase)),16)*(1-smoothstep(.15,1.2,fwidth(phase)));
diamond*=pow(1-Vacuum,2)*(1-Mix);
float core=1-smoothstep(.18,.64,z);
float3 col=lerp(float3(1,.46,.16),float3(.72,.82,1),core);
col=lerp(col,float3(1,.64,.34),Mix);
col=lerp(col,float3(.92,.93,1),diamond*.8);
return col*(lerp(7200,13500,Mix)*(.7+.5*N.r)+diamond*21000)*Throttle;
''')
connect(m, emission, u.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = custom(m, inputs, '''
float z=UV.y;
float envelope=smoothstep(0,.018,z)*pow(saturate(1-z),1.6);
float breakup=smoothstep(lerp(.10,.43,z),.70,N.r*.7+N.g*.3);
return envelope*breakup*lerp(.48,.24,Mix)*(1-.65*Vacuum*Mix);
''', u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m, opacity, u.MaterialProperty.MP_OPACITY)
normal = expression(m, u.MaterialExpressionVertexNormalWS)
wpo = custom(m, {**inputs, 'Normal': normal}, '''
return Normal*(N.b-.5)*sin(UV.y*3.14159265)*lerp(30,160,Mix);
''')
connect(m, wpo, u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
save(m)
print('PROPULSION_MATERIALS_READY', flush=True)

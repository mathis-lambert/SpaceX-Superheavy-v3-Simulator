"""Spherical, world-anchored marine / mid / cirrus cloud density."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,prop,material,expression,scalar,vector,custom,color,sample,connect,save,E

m=material('/Game/Starbase/Materials/M_LayeredWeather')
prop(m,'blend_mode',u.BlendMode.BLEND_ADDITIVE);prop(m,'material_domain',u.MaterialDomain.MD_VOLUME)
prop(m,'used_with_volumetric_cloud',True)
p=expression(m,u.MaterialExpressionWorldPosition);time=expression(m,u.MaterialExpressionTime)
wind=vector(m,'WindMps',(8,3,0,0))
uv=custom(m,{'P':p,'T':time,'W':wind},'return (P*.01-W.xyz*T)/180000;')
noise='/Game/Starbase/Textures/Effects/T_FlowNoise128'
large=sample(m,noise,uv,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
uv2=custom(m,{'UV':uv,'N':large},'return UV*41.37+(N.rgb-.5)*2.7+float3(.31,.57,.18);')
medium=sample(m,noise,uv2,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
uv3=custom(m,{'UV':uv,'N':medium},'return UV*173.19+(N.rgb-.5)*.39;')
fine=sample(m,noise,uv3,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
for node in (large,medium,fine):
    prop(node,'mip_value_mode',u.TextureMipValueMode.TMVM_MIP_LEVEL);prop(node,'const_mip_value',0)
d=custom(m,{'P':p,'N':large,'M':medium,'F':fine,'C':scalar(m,'Coverage',.48),'High':scalar(m,'HighClouds',.65)},'''
float h=(length(P+float3(0,0,637100000))-637100000)*.01;
float marine=smoothstep(650,1000,h)*(1-smoothstep(1800,2600,h));
float middle=smoothstep(3500,4000,h)*(1-smoothstep(5000,6200,h));
float cirrus=smoothstep(7900,8200,h)*(1-smoothstep(9600,10300,h));
float weather=smoothstep(.65-C*.45,.8-C*.38,N.r);
float shape=saturate((M.r*.7+F.g*.3)-(.52-C*.18));
float low=weather*shape*marine;
float mid=smoothstep(.48,.7,N.g)*saturate(M.b*.7+F.r*.3-.5)*middle;
float high=smoothstep(.5,.72,N.b)*smoothstep(.45,.7,M.g)*cirrus;
return (low*.08+mid*.025+high*.004*High)*.01;
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m,d,u.MaterialProperty.MP_SUBSURFACE_COLOR)
connect(m,color(m,(.98,.985,1)),u.MaterialProperty.MP_BASE_COLOR)
connect(m,color(m,(0,0,0)),u.MaterialProperty.MP_EMISSIVE_COLOR)
advanced=expression(m,u.MaterialExpressionVolumetricAdvancedMaterialOutput)
prop(advanced,'multi_scattering_approximation_octave_count',1)
prop(advanced,'ground_contribution',True)
prop(advanced,'const_phase_g',.5)
prop(advanced,'gray_scale_material',True)
# A cheap conservative test skips empty altitude bands before sampling 3D noise.
occupied=custom(m,{'P':p},'''
float h=(length(P+float3(0,0,637100000))-637100000)*.01;
return float3((h>600 && h<2650)||(h>3450 && h<6250)||(h>7850 && h<10350),0,0);
''')
inputs=E.get_material_expression_input_names(advanced)
pin=next(name for name in inputs if 'Conservative' in str(name))
assert E.connect_material_expressions(occupied,'',advanced,pin)
save(m)
print('LAYERED_WEATHER_READY',flush=True)

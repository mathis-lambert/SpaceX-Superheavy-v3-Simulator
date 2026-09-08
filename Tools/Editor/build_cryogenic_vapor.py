"""Two continuous, self-shadowed heterogeneous volumes for cold condensation.

Uses the authored 128-cubed field; no sprite sheets or overlapping fog spheres.
The procedural density is an optical approximation, not a fluid simulation.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,material,prop,expression,scalar,vector,color,custom,sample,connect,save

m=material('/Game/Starbase/Materials/M_CryogenicVapor')
prop(m,'blend_mode',u.BlendMode.BLEND_ADDITIVE)
prop(m,'material_domain',u.MaterialDomain.MD_VOLUME)
prop(m,'used_with_heterogeneous_volumes',True)
p=expression(m,u.MaterialExpressionWorldPosition)
origin=vector(m,'VentPosition',(0,0,0,1))
up=vector(m,'Up',(0,0,1,1))
side=vector(m,'Outward',(1,0,0,1))
wind=vector(m,'Wind',(0,0,0,1))
time=scalar(m,'FlowTime',0)
strength=scalar(m,'FlowStrength',0)
local=custom(m,{'P':p,'V':origin,'U':up,'O':side,'W':wind},'''
float3 q=(P-V)*.01;
float down=-dot(q,U.xyz);
float travel=max(down,0)/2.5;
float3 drift=W.xyz*.01*(travel-.9*(1-exp(-travel/.9)))*.2;
q-=drift;
float3 tangent=normalize(cross(U.xyz,O.xyz));
return float3(dot(q,O.xyz),dot(q,tangent),down);
''')
uv=custom(m,{'Q':local,'T':time},'return Q*float3(.21,.21,.16)+float3(0,0,-T*.46);')
n=sample(m,'/Game/Starbase/Textures/Effects/T_FlowNoise128',uv,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
fineuv=custom(m,{'UV':uv,'N':n},'return UV*3.07+(N.rgb-.5)*.44;')
fine=sample(m,'/Game/Starbase/Textures/Effects/T_FlowNoise128',fineuv,sampler=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
for node in (n,fine):
    prop(node,'mip_value_mode',u.TextureMipValueMode.TMVM_MIP_LEVEL);prop(node,'const_mip_value',0)
d=custom(m,{'Q':local,'N':n,'F':fine,'S':strength},'''
float depth=max(Q.z,0);
float width=.65+depth*.16;
// Detached, curling lobes follow the downward flow; density fades before bounds.
float2 q=Q.xy-float2(.22+depth*.048+(N.g-.5)*width*.9,(N.b-.5)*width*.9);
float envelope=1-smoothstep(width*.35,width*1.5,length(q));
float top=smoothstep(-.2,.35,Q.z);
float tail=1-smoothstep(17,28,Q.z);
float curls=smoothstep(.29,.67,N.r*.65+F.g*.35);
// No condensation inside the cylindrical hull (vent is at its surface).
float hull=smoothstep(-.18,.08,Q.x+Q.y*Q.y/9.3);
return S*.12*envelope*top*tail*curls*hull/(1+depth*.07);
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m,d,u.MaterialProperty.MP_SUBSURFACE_COLOR)
connect(m,color(m,(.94,.965,.98)),u.MaterialProperty.MP_BASE_COLOR)
connect(m,color(m,(0,0,0)),u.MaterialProperty.MP_EMISSIVE_COLOR)
save(m)
print('CRYOGENIC_VOLUME_READY')

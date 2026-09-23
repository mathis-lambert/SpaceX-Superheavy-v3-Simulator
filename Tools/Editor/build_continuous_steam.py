"""Two explicitly streamed density frames with spatially absorbing boundaries."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,prop,material,expression,scalar,vector,custom,color,connect,save,E

m=material('/Game/Starbase/Materials/Effects/M_TurbulentDeluge')
prop(m,'blend_mode',u.BlendMode.BLEND_ADDITIVE)
prop(m,'material_domain',u.MaterialDomain.MD_VOLUME)
prop(m,'used_with_heterogeneous_volumes',True)
svt=u.load_asset('/Game/Starbase/FX/Volumes/SVT_TurbulentDeluge');assert svt
p=expression(m,u.MaterialExpressionWorldPosition)
uv=custom(m,{'P':p,'C':vector(m,'VolumeCentre',(0,0,0,1)),
 'X':vector(m,'VolumeAxisX',(1,0,0,1)),'Y':vector(m,'VolumeAxisY',(0,1,0,1)),
 'Z':vector(m,'VolumeAxisZ',(0,0,1,1)),'S':vector(m,'VolumeSize',(2000,1600,1200,1))},
 'float3 q=P-C.xyz;return float3(dot(q,X.xyz),dot(q,Y.xyz),dot(q,Z.xyz))/max(S.xyz,1)+.5;')
fields=[]
for name in ('DensityVolume','DensityNext'):
    node=expression(m,u.MaterialExpressionSparseVolumeTextureSampleParameter)
    prop(node,'parameter_name',name);prop(node,'sparse_volume_texture',svt)
    assert E.connect_material_expressions(uv,'',node,E.get_material_expression_input_names(node)[0])
    fields.append(node)
d=custom(m,{'A':(fields[0],'Attributes A'),'B':(fields[1],'Attributes A'),
 'Blend':scalar(m,'FrameBlend',0),'UV':uv,'S':scalar(m,'DensityScale',1),
 'V':scalar(m,'MetersPerVoxel',.15625)},'''
float3 edge=min(UV,1-UV);
float boundary=smoothstep(0,.14,min(edge.x,min(edge.y,edge.z)));
return max(lerp(A.r,B.r,saturate(Blend)),0)*S*V*3*boundary;
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m,d,u.MaterialProperty.MP_SUBSURFACE_COLOR)
connect(m,color(m,(.92,.95,.975)),u.MaterialProperty.MP_BASE_COLOR)
connect(m,color(m,(0,0,0)),u.MaterialProperty.MP_EMISSIVE_COLOR)
save(m)
print('CONTINUOUS_STEAM_READY',flush=True)

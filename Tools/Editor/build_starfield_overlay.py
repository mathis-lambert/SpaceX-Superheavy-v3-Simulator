"""Stars overlay the native planetary atmosphere instead of replacing its sky."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,prop,material,expression,custom,connect,save
m=material('/Game/Starbase/Materials/M_StarField')
prop(m,'shading_model',u.MaterialShadingModel.MSM_UNLIT)
prop(m,'is_sky',False)
prop(m,'blend_mode',u.BlendMode.BLEND_ADDITIVE)
prop(m,'two_sided',True)
prop(m,'use_translucency_vertex_fog',False)
p=expression(m,u.MaterialExpressionWorldPosition)
c=expression(m,u.MaterialExpressionCameraPositionWS)
stars=custom(m,{'P':p,'C':c},'''
float3 d=normalize(P-C);
float2 uv=float2(atan2(d.y,d.x)/6.2831853+.5,acos(clamp(d.z,-1,1))/3.14159265);
float2 grid=uv*float2(1440,720),cell=floor(grid),f=frac(grid);
float3 h=frac(cell.xyx*float3(.1031,.1030,.0973));h+=dot(h,h.yxz+33.33);h=frac((h.xxy+h.yxx)*h.zyx);
float r=length(f-(.2+h.xy*.6)),aa=max(length(fwidth(grid)),.025);
float star=(1-smoothstep(.07,.07+aa*.6,r))*step(.996,h.z);
return star*lerp(float3(.65,.8,1),float3(1,.8,.58),h.x)*220;
''')
connect(m,stars,u.MaterialProperty.MP_EMISSIVE_COLOR)
save(m)
print('STARFIELD_OVERLAY_READY',flush=True)

"""Tune only current propulsion materials, preserving their engine-facing inputs."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,E,prop,save

def prune_unused(m):
    roots=[]
    for prop_name in ['MP_BASE_COLOR','MP_EMISSIVE_COLOR','MP_OPACITY','MP_WORLD_POSITION_OFFSET',
                      'MP_SUBSURFACE_COLOR','MP_ROUGHNESS','MP_METALLIC','MP_NORMAL','MP_SPECULAR']:
        node=E.get_material_property_input_node(m,getattr(u.MaterialProperty,prop_name))
        if node: roots.append(node)
    reachable=set();pending=list(roots)
    while pending:
        node=pending.pop()
        if node in reachable:continue
        reachable.add(node)
        pending.extend(n for n in E.get_inputs_for_material_expression(m,node) if n)
    assert roots and reachable
    for node in E.get_material_expressions(m):
        if node not in reachable and not isinstance(node,u.MaterialExpressionCustomOutput):
            E.delete_material_expression(m,node)


m=u.load_asset('/Game/Starbase/Materials/M_RaptorPlume');assert m
changed=0
active_emission=E.get_material_property_input_node(m,u.MaterialProperty.MP_EMISSIVE_COLOR)
for node in E.get_material_expressions(m):
    if not isinstance(node,u.MaterialExpressionCustom):continue
    code=node.get_editor_property('code')
    if node==active_emission and ('float3 core=' in code or 'float spacing=' in code):
        prefix=code[:code.index('float z=')]
        prop(node,'code',prefix+'''
float z=UV.y;
float circumference=sin(UV.x*6.283185)*2.5+cos(UV.x*6.283185)*3.1;
float n=N.f(float2(circumference,z*16-T*9));
float spacing=lerp(54,78,sqrt(saturate(Throttle)))-28*Vacuum;
float diamond=pow(saturate(sin(z*spacing-.4)),16)*pow(1-Vacuum,2)*(1-Mix);
float core=1-smoothstep(.18,.64,z);
float3 col=lerp(float3(1,.46,.16),float3(.72,.82,1),core);
col=lerp(col,float3(1,.64,.34),Mix);col=lerp(col,float3(.92,.93,1),diamond*.8);
return col*(lerp(7200,13500,Mix)*(.65+.5*n)+diamond*21000)*Throttle;
''');changed+=1
    elif 'float edge=' in code:
        prop(node,'code',code.replace('UV.x*12','sin(UV.x*6.283185)*3+cos(UV.x*6.283185)*4'))
    elif 'Normal*' in code:
        prop(node,'code',code.replace('UV.x*8','sin(UV.x*6.283185)*2+cos(UV.x*6.283185)*3'))
assert changed==1,'Unexpected flame graph; do not silently replace it'
prune_unused(m)
save(m)

# Preserve the physically lit medium. Break up the density edge through domain
# warping, rather than piling up additional large spheres or emissive smoke.
m=u.load_asset('/Game/Starbase/Materials/M_VolumetricVapor');assert m
changed=0
for node in E.get_material_expressions(m):
    if not isinstance(node,u.MaterialExpressionCustom):continue
    code=node.get_editor_property('code')
    if 'float envelope =' in code or 'float3 warp=' in code:
        prop(node,'code','''
float3 warp=(N.gbr-.5)*.38+(F.brg-.5)*.12;
float envelope=saturate((1-length(Q+warp))/.30);
float field=N.r*.62+N.g*.25+F.b*.13;
float erosion=(1-envelope)*.27;
float shape=smoothstep(.33,.63,field-erosion);
return D*.105*shape*envelope;
''');changed+=1
assert changed==1,'Unexpected vapor graph'
prune_unused(m)
save(m)
print('PROPULSION_MATERIALS_READY')

"""Filter authored steel detail and align planetary atmosphere registration."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u,prop,material,expression,custom,constant,connect,save
m=material('/Game/Starbase/Materials/M_StainlessFlight')
uv=expression(m,u.MaterialExpressionTextureCoordinate)
base=custom(m,{'UV':uv},'''
float phase=UV.y*54/1.82*3.14159265;
float footprint=max(fwidth(phase),.0001);
float weld=pow(abs(cos(phase)),55)*(1-smoothstep(.1,.8,footprint));
float brushPhase=UV.x*900;
float brush=sin(brushPhase)*.006*(1-smoothstep(.2,2,fwidth(brushPhase)));
return float3(.56,.59,.62)*(1-weld*.14+brush);
''')
rough=custom(m,{'UV':uv},'''
float p=UV.y*72;
float filtered=sin(p)*sin(UV.x*24)*(1-smoothstep(.1,1.5,length(fwidth(UV*float2(24,72)))));
return .3+.025*filtered;
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
connect(m,base,u.MaterialProperty.MP_BASE_COLOR);connect(m,constant(m,1),u.MaterialProperty.MP_METALLIC)
connect(m,rough,u.MaterialProperty.MP_ROUGHNESS);save(m)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    sky=a.get_component_by_class(u.SkyAtmosphereComponent)
    if sky:
        prop(sky,'bottom_radius',6370.99);prop(sky,'atmosphere_height',100.01)
        prop(sky,'trace_sample_count_scale',2.)
        prop(sky,'mie_scattering_scale',1.)
        prop(sky,'mie_exponential_distribution',1.2)
    sun=a.get_component_by_class(u.DirectionalLightComponent)
    if sun and sun.get_editor_property('atmosphere_sun_light'):
        prop(sun,'per_pixel_atmosphere_transmittance',True)
assert levels.save_current_level()
print('WORLD_FINISH_READY',flush=True)

"""Rebuild only the RCS optical material."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from unreal_materials import u, material, prop, expression, scalar, custom, connect, save


def build_attitude_gas():
    gas=material('/Game/Starbase/Materials/M_AttitudeGas')
    prop(gas,'blend_mode',u.BlendMode.BLEND_ADDITIVE)
    prop(gas,'shading_model',u.MaterialShadingModel.MSM_UNLIT)
    prop(gas,'two_sided',True)
    uv=expression(gas,u.MaterialExpressionTextureCoordinate)
    power=scalar(gas,'Power',0)
    time=scalar(gas,'Time',0)
    # UE's imported V coordinate runs from the nozzle to the tail. Angular
    # frequencies are integral so the mesh UV seam never cuts through the gas.
    radiance=custom(gas,{'UV':uv,'T':time},'''
float z=saturate(UV.y),a=UV.x*6.2831853;
float shear=.72+.17*sin(z*23-T*17+sin(a*3+z*4))+.11*sin(z*51-T*29+a*7);
float core=pow(1-z,1.7);
return lerp(float3(.72,.82,.94),float3(.9,.94,1),z)*75000*core*shear;''')
    opacity=custom(gas,{'UV':uv,'P':power},'''
float z=saturate(UV.y);
return .34*P*smoothstep(0,.018,z)*pow(1-z,.65);''',u.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(gas,radiance,u.MaterialProperty.MP_EMISSIVE_COLOR)
    connect(gas,opacity,u.MaterialProperty.MP_OPACITY)
    save(gas)
    return gas

build_attitude_gas()
print('ATTITUDE_GAS_READY')

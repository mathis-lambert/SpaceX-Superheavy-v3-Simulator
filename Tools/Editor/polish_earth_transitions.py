"""Fade historic orthophoto coverage into regional imagery at high altitude."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u
E=u.MaterialEditingLibrary;A=u.EditorAssetLibrary
names=['M_BocaRegion']+[f'M_BocaChica_{i}_{j}' for j in range(4) for i in range(4)]
for name in names:
    m=u.load_asset('/Game/Starbase/Materials/Earth/'+name);assert m,name
    nodes=E.get_material_expressions(m)
    if any(isinstance(n,u.MaterialExpressionCustom) and n.get_editor_property('description')=='Altitude imagery transition' for n in nodes):continue
    previous=E.get_material_property_input_node(m,u.MaterialProperty.MP_BASE_COLOR)
    regional=next(n for n in nodes if isinstance(n,u.MaterialExpressionTextureSample) and n.texture.get_name()=='T_GulfRegion')
    camera=E.create_material_expression(m,u.MaterialExpressionCameraPositionWS)
    x=E.create_material_expression(m,u.MaterialExpressionCustom)
    x.set_editor_property('description','Altitude imagery transition');x.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    pins=[]
    for label in ['C','B','Camera']:
        p=u.CustomInput();p.set_editor_property('input_name',label);pins.append(p)
    x.set_editor_property('inputs',pins)
    x.set_editor_property('code','float altitude=length(Camera+float3(0,0,637100000))-637100000;float near=1-smoothstep(350000,1600000,altitude);return lerp(B,C,near);')
    assert E.connect_material_expressions(previous,'',x,'C')
    assert E.connect_material_expressions(regional,'RGB',x,'B')
    assert E.connect_material_expressions(camera,'',x,'Camera')
    assert E.connect_material_property(x,'',u.MaterialProperty.MP_BASE_COLOR)
    E.recompile_material(m);assert A.save_loaded_asset(m,False)
print('EARTH_ALTITUDE_TRANSITIONS_READY')

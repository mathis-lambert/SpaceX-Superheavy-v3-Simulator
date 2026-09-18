
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
import unreal as u
E=u.MaterialEditingLibrary
for path in ['/Game/Starbase/Vehicle/Materials/SuperheavyBody','/Game/Starbase/Materials/M_BoosterFlight','/Game/Starbase/Materials/M_CryogenicVapor','/Game/Starbase/Materials/M_RaptorPlume']:
    m=u.load_asset(path)
    print('PROBE_MATERIAL',path,m)
    if not m:continue
    for p in [u.MaterialProperty.MP_BASE_COLOR,u.MaterialProperty.MP_METALLIC,u.MaterialProperty.MP_ROUGHNESS]:
        node=E.get_material_property_input_node(m,p);print('INPUT',str(p),node)
    for n in E.get_material_expressions(m):
        if isinstance(n,u.MaterialExpressionScalarParameter):print('SCALAR',n.get_editor_property('parameter_name'),n.get_editor_property('default_value'))
        if isinstance(n,u.MaterialExpressionVectorParameter):print('VECTOR',n.get_editor_property('parameter_name'),n.get_editor_property('default_value'))
        if isinstance(n,u.MaterialExpressionTextureSample):print('TEXTURE',n.get_name(),n.texture)
for n in ['T_EarthSeptember','T_RegionalContinuous','T_BocaChica_0_0']:
    t=u.load_asset('/Game/Starbase/Textures/Earth/'+n)
    print('TEXTURE_RESOLUTION',n,t.blueprint_get_size_x(),t.blueprint_get_size_y(),t.get_editor_property('max_texture_size'))
for s in ['audit_recovery_assets.py','audit_earth_assets.py','audit_recovery_vfx.py']:
    exec((Path(u.Paths.project_dir())/'Tools'/'Tests'/s).read_text(encoding='utf-8'))

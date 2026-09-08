"""Read-only final material and Niagara asset audit, suitable for commandlets."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u, json, struct
from pathlib import Path
E=u.MaterialEditingLibrary
report={'success':False,'materials':{},'atlas':{},'system':{}}
for name in ['M_VolumetricVapor','M_CryogenicVapor','M_VaporTrail','M_RaptorPlume']:
    m=u.load_asset('/Game/Starbase/Materials/'+name)
    assert m,name
    report['materials'][name]={'blend':str(m.get_editor_property('blend_mode')),'shading':str(m.get_editor_property('shading_model'))}
    if name=='M_CryogenicVapor':
        assert m.get_editor_property('material_domain')==u.MaterialDomain.MD_VOLUME
        assert m.get_editor_property('used_with_heterogeneous_volumes')
        assert E.get_material_property_input_node(m,u.MaterialProperty.MP_SUBSURFACE_COLOR)
texture=u.load_asset('/Game/Starbase/Textures/T_RecoveryVaporAtlas')
assert texture
report['atlas']={'width':texture.blueprint_get_size_x(),'height':texture.blueprint_get_size_y(),'srgb':texture.get_editor_property('srgb'),'compression':str(texture.get_editor_property('compression_settings'))}
source=ART_ROOT/'Flight'/'T_RecoveryVaporAtlas.png'
report['atlas']['source_size']=list(struct.unpack('>II',source.read_bytes()[16:24]))
assert report['atlas']['source_size']==[2048,2048]
report['atlas']['resource_size_may_be_streaming']=True
system=u.load_asset('/Game/Starbase/FX/NS_RecoveryVaporTrail')
assert system
report['system']['path']=system.get_path_name()
report['success']=True
target=Path(u.Paths.project_saved_dir())/'Recovery'/'vfx-asset-audit.json'
target.write_text(json.dumps(report,indent=2),encoding='utf-8')
print('RECOVERY_VFX_AUDIT_OK')

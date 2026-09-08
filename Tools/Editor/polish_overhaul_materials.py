"""Correct packed roughness/metalness channels without changing the source material."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import unreal as u
E=u.MaterialEditingLibrary;A=u.EditorAssetLibrary
dst='/Game/Starbase/Materials/M_BoosterFlight'
m=u.load_asset(dst) if A.does_asset_exist(dst) else A.duplicate_asset('/Game/Starbase/Vehicle/Materials/SuperheavyBody',dst)
samples=[n for n in E.get_material_expressions(m) if isinstance(n,u.MaterialExpressionTextureSample)]
rough=next(n for n in samples if '_channels_G' in n.texture.get_name() or 'BoosterRough' in n.texture.get_name())
metal=next(n for n in samples if '_channels_B' in n.texture.get_name() or 'BoosterMetal' in n.texture.get_name())
# glTF metallic-roughness convention: G is roughness, B is metalness.
for n,channel,name in [(rough,'G','T_BoosterRoughLinear'),(metal,'B','T_BoosterMetalLinear')]:
    original='/Game/Starbase/Vehicle/Textures/Superheavy-V3_Metallic-Superheavy-V3_Roughness_channels_'+channel
    source=u.load_asset(original);source.set_editor_property('srgb',True);assert A.save_loaded_asset(source,False),original
    target='/Game/Starbase/Textures/'+name
    t=u.load_asset(target) if A.does_asset_exist(target) else A.duplicate_asset(original,target)
    t.set_editor_property('srgb',False);assert A.save_loaded_asset(t,False),target;n.set_editor_property('texture',t)
    n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE if t.get_editor_property('compression_settings')==u.TextureCompressionSettings.TC_GRAYSCALE else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
assert E.connect_material_property(metal,'R',u.MaterialProperty.MP_METALLIC)
assert E.connect_material_property(rough,'R',u.MaterialProperty.MP_ROUGHNESS)
E.recompile_material(m);assert A.save_loaded_asset(m,False)
print('OVERHAUL_STEEL_CHANNELS_CORRECTED')

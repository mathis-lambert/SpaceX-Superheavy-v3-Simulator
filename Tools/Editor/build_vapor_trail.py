"""Canonical lit condensation trail. Atlas sprites complement nearby volumes."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Shared'))
from unreal_materials import u, prop, material, expression, custom, vector, constant, sample, connect, save

m = material('/Game/Starbase/Materials/M_VaporTrail')
prop(m, 'blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
prop(m, 'shading_model', u.MaterialShadingModel.MSM_DEFAULT_LIT)
prop(m, 'translucency_lighting_mode', u.TranslucencyLightingMode.TLM_VOLUMETRIC_DIRECTIONAL)
prop(m, 'two_sided', True)
prop(m, 'used_with_niagara_sprites', True)
uv = expression(m, u.MaterialExpressionTextureCoordinate)
seed = expression(m, u.MaterialExpressionParticleRandom)
age = expression(m, u.MaterialExpressionParticleRelativeTime)
particle = expression(m, u.MaterialExpressionParticleColor)
coords = custom(m, {'UV': uv, 'Seed': seed, 'Age': age}, '''
float id=floor(frac(Seed*.6180339)*16);
float2 q=UV+.018*sin(UV.yx*18+float2(Seed*13+Age*.5,Seed*5-Age*.35))*sin(UV*3.14159);
return (clamp(q,.008,.992)+float2(fmod(id,4),floor(id/4)))/4;
''', u.CustomMaterialOutputType.CMOT_FLOAT2)
tex = sample(m, '/Game/Starbase/Textures/T_RecoveryVaporAtlas', coords,
             sampler=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
tint = vector(m, 'Tint', (.88, .94, 1, 1))
connect(m, custom(m, {'Shade': tex, 'Tint': tint}, 'return Tint*(.38+.62*Shade.r);'), u.MaterialProperty.MP_BASE_COLOR)
connect(m, constant(m, 0), u.MaterialProperty.MP_EMISSIVE_COLOR)
connect(m, constant(m, 1), u.MaterialProperty.MP_ROUGHNESS)
density = custom(m, {'A': (tex, 'A'), 'Alpha': (particle, 'A'), 'Age': age},
                 'return saturate(A*Alpha*smoothstep(0,.018,Age)*pow(saturate(1-Age),1.1)*.42);',
                 u.CustomMaterialOutputType.CMOT_FLOAT1)
depth = expression(m, u.MaterialExpressionDepthFade)
prop(depth, 'fade_distance_default', 180)
assert u.MaterialEditingLibrary.connect_material_expressions(density, '', depth, 'Opacity')
connect(m, depth, u.MaterialProperty.MP_OPACITY)
save(m)
print('VAPOR_TRAIL_READY', flush=True)

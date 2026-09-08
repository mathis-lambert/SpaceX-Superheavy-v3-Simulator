"""Refresh legacy texture references in copied material graphs and shader caches."""
import unreal as u,json
from pathlib import Path
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary;R=u.AssetRegistryHelpers.get_asset_registry();R.search_all_assets(True)
opts=u.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True)
oldpaths=['/Game/SuperHeavy/Textures/Superheavy-V3_BaseColor','/Game/SuperHeavy/Textures/Superheavy-V3_Normal']
refs=set(str(p) for old in oldpaths for p in (R.get_referencers(old,opts) or []))
report=[]
for path in sorted(refs):
    m=u.load_asset(path);assert isinstance(m,u.Material),path
    for n in E.get_material_expressions(m):
        if not isinstance(n,u.MaterialExpressionTextureSample):continue
        texture=n.get_editor_property('texture')
        if texture is None:
            sampler=n.get_editor_property('sampler_type')
            assert sampler in [u.MaterialSamplerType.SAMPLERTYPE_NORMAL,u.MaterialSamplerType.SAMPLERTYPE_COLOR],str(sampler)
            suffix='Normal' if sampler==u.MaterialSamplerType.SAMPLERTYPE_NORMAL else 'BaseColor'
            texture=u.load_asset('/Game/Starbase/Vehicle/Textures/Superheavy-V3_'+suffix);assert texture
            n.set_editor_property('texture',texture)
        report.append({'material':path,'node':n.get_name(),'texture':texture.get_path_name()})
    E.recompile_material(m);assert A.save_loaded_asset(m,False),path
Path(u.Paths.project_saved_dir(),'Recovery/material-texture-repair.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('MATERIAL_REFERENCES_REFRESHED',len(refs))

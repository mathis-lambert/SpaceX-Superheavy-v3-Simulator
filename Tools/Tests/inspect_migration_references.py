import unreal as u,json
from pathlib import Path
A=u.EditorAssetLibrary;R=u.AssetRegistryHelpers.get_asset_registry()
R.search_all_assets(True)
opts=u.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True)
report=[]
for prefix in ['/Game/Recovery','/Game/MWLandscapeAutoMaterial','/Game/SuperHeavy']:
    for p in A.list_assets(prefix,True,False):
        d=A.find_asset_data(p)
        report.append({'path':p,'class':str(d.asset_class_path),'references':[str(x) for x in (R.get_referencers(str(d.package_name),opts) or [])]})
Path(u.Paths.project_saved_dir(),'Recovery/migration-leftovers.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('MIGRATION_LEFTOVERS',json.dumps(report))

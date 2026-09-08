"""One-time asset migration, using Unreal's reference-aware rename API."""
import unreal as u,json
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve()
backup=root/'Saved/Recovery/BeforeRestructure/manifest.json'
assert backup.exists(), 'A verified complete recovery point is required'
A=u.EditorAssetLibrary
moves=[('/Game/Recovery','/Game/Starbase'),
       ('/Game/SuperHeavy','/Game/Starbase/Vehicle'),
       ('/Game/MWLandscapeAutoMaterial','/Game/ThirdParty/MWLandscapeAutoMaterial'),
       ('/Game/WaterMaterials','/Game/ThirdParty/WaterMaterials'),
       ('/Game/SimBlank','/Game/Archive/SimulationTemplate'),
       ('/Game/Maps','/Game/Archive/PrototypeMaps')]
manifest=[]
for source,target in moves:
    pending=[]
    for path in A.list_assets(source,True,False):
        asset=A.find_asset_data(path)
        if str(asset.asset_class_path.asset_name)=='ObjectRedirector':continue
        destination=path.replace(source,target,1)
        manifest.append({'from':path,'to':destination})
        if not A.does_asset_exist(destination):
            obj=u.load_asset(path)
            if obj is None:
                # This pre-existing phase profile references a deleted C++ class.
                # Preserve the original package separately; it is not a runtime asset.
                assert '/DA_SuperHeavy_PhaseProfile.' in path,path
                manifest[-1]['unloadable_legacy_class']='SuperHeavyFlightPhaseProfile'
                print('LEGACY_UNLOADABLE_PRESERVED',path)
                continue
            package=destination.split('.')[0]
            pending.append(u.AssetRenameData(obj,package.rsplit('/',1)[0],package.rsplit('/',1)[1]))
    if pending:assert u.AssetToolsHelpers.get_asset_tools().rename_assets(pending),(source,target)
    print('CONTENT_FOLDER_READY',source,target)
old='/Game/Starbase/Vehicle/Blueprints/BP_SuperHeavy_Backup'
new='/Game/Archive/Vehicle/Blueprints/BP_SuperHeavy_Backup'
if A.does_asset_exist(old) and not A.does_asset_exist(new):assert A.rename_asset(old,new)
for name in ['M_Graphite','M_Cladding','M_SafetyAmber','M_SiteLamp']:
    m=u.load_asset('/Game/Starbase/Materials/'+name)
    m.set_editor_property('used_with_nanite',True)
    u.MaterialEditingLibrary.recompile_material(m);assert A.save_loaded_asset(m,False)
# Source import records follow the authored files into ArtSource. No mesh is reimported.
source_moves={'assets/3d':'ArtSource/Originals','assets/earth':'ArtSource/Earth',
              'assets/recovery':'ArtSource/Flight','assets/overhaul':'ArtSource/VehicleDetails',
              'assets/audio':'ArtSource/Audio'}
for path in A.list_assets('/Game/Starbase',True,False):
    obj=u.load_asset(path)
    try:data=obj.get_editor_property('asset_import_data')
    except Exception:continue
    if not data:continue
    files=list(data.extract_filenames())
    for index,file in enumerate(files):
        normalized=file.replace('\\','/')
        for source,target in source_moves.items():normalized=normalized.replace(source,target)
        if normalized!=file.replace('\\','/'):
            data.scripted_add_filename(normalized,index,'')
            assert A.save_loaded_asset(obj,False)
(root/'Saved/Recovery/content-migration.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('CONTENT_RESTRUCTURE_READY',len(manifest))

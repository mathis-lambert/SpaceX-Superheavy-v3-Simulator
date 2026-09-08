"""Consolidate packages retained by volumetric-cloud references during rename."""
import unreal as u,json
from pathlib import Path
P=Path(u.Paths.project_dir());A=u.EditorAssetLibrary
assert (P/'Saved/Recovery/BeforeRestructure/manifest.json').exists()
items=json.loads((P/'Saved/Recovery/migration-leftovers.json').read_text())
result=[]
for item in items:
    old=item['path']
    if old.startswith('/Game/SuperHeavy/'):continue
    new=old.replace('/Game/Recovery/','/Game/Starbase/').replace('/Game/MWLandscapeAutoMaterial/','/Game/ThirdParty/MWLandscapeAutoMaterial/')
    source=u.load_asset(old);target=u.load_asset(new)
    assert source and target,(old,new)
    if source==target:continue
    assert source.get_class()==target.get_class(),old
    assert A.consolidate_assets(target,[source]),old
    result.append({'from':old,'to':new})
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
instance=u.load_asset('/Game/Starbase/Materials/MI_CloudFlight')
u.MaterialEditingLibrary.set_material_instance_parent(instance,u.load_asset('/Game/Starbase/Materials/M_CloudFlight'))
assert A.save_loaded_asset(instance,False)
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='Recovery_Clouds':a.get_component_by_class(u.VolumetricCloudComponent).set_editor_property('material',instance)
levels.save_current_level()
u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(P/'Saved/Recovery/material-consolidation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print('MIGRATED_MATERIALS_CONSOLIDATED',len(result))

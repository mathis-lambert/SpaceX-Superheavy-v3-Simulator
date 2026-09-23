"""Validate the reorganized runtime dependency graph and visual/physics contracts."""
import sys,json,re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'/'Shared'))
from project_paths import PROJECT_ROOT,CONTENT_ROOT
import unreal as u
A=u.EditorAssetLibrary;R=u.AssetRegistryHelpers.get_asset_registry()
R.search_all_assets(True)
root=CONTENT_ROOT;report={'success':False,'blueprints':[],'nanite_meshes':[],'dependencies':[]}
for file in ['audit_recovery_assets.py','audit_earth_assets.py','audit_recovery_vfx.py']:
    namespace={'__file__':str(Path(__file__).parent/file)}
    exec(compile((Path(__file__).parent/file).read_text(encoding='utf-8'),file,'exec'),namespace)
for p in A.list_assets(root,True,False):
    data=A.find_asset_data(p)
    if str(data.asset_class_path.asset_name)=='Blueprint':
        bp=u.load_asset(p);assert bp and bp.generated_class(),p
        u.BlueprintEditorLibrary.compile_blueprint(bp)
        assert str(bp.get_editor_property('status')) not in ['BlueprintStatus.BS_ERROR'],p
        report['blueprints'].append(p)
for prefix in [root+'/Vehicle/Meshes/Imported_Clean']:
    for p in A.list_assets(prefix,True,False):
        m=u.load_asset(p);assert m
        if not isinstance(m,u.StaticMesh):continue
        assert m.get_editor_property('nanite_settings').get_editor_property('enabled'),p
        body=m.get_editor_property('body_setup')
        assert body.get_editor_property('collision_trace_flag')==u.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX,p
        report['nanite_meshes'].append(p)
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
assert not any(a.get_actor_label() in ['Recovery_LaunchPad','Recovery_CatchPad'] for a in actors)
cloud=next(a for a in actors if a.get_actor_label()=='Recovery_Clouds').get_component_by_class(u.VolumetricCloudComponent)
assert cloud.get_editor_property('material').get_path_name()==root+'/Materials/M_LayeredWeather.M_LayeredWeather'
for name in ['S_EngineRoar','S_CoastalWind','S_EngineRumble','S_EngineCrackle','S_CryogenicHiss','S_Deluge','S_TowerDrive']:
    sound=u.load_asset(root+'/Audio/'+name);assert sound and sound.get_editor_property('looping'),name
    assert sound.get_editor_property('virtualization_mode')==u.VirtualizationMode.PLAY_WHEN_SILENT,name
for name in ['S_TowerContact','S_MountRelease']:
    sound=u.load_asset(root+'/Audio/'+name);assert sound and not sound.get_editor_property('looping'),name
    assert sound.get_editor_property('duration')>1,name
    assert sound.get_editor_property('loading_behavior')==u.SoundWaveLoadingBehavior.FORCE_INLINE,name
for name in ('M_Cladding','M_Graphite','M_Concrete','M_SafetyAmber'):
    mat=u.load_asset(root+'/Materials/'+name)
    assert mat.get_editor_property('used_with_instanced_static_meshes'),f'{name}: site details would use the fallback material'
vapor=u.load_asset(root+'/Materials/M_VolumetricVapor')
assert vapor.get_editor_property('material_domain')==u.MaterialDomain.MD_VOLUME
flow=u.load_asset(root+'/FX/Volumes/SVT_TurbulentDeluge')
assert flow and flow.get_num_frames()==64,'Turbulent flow sequence is incomplete'
flow_material=u.load_asset(root+'/Materials/Effects/M_TurbulentDeluge')
assert flow_material and flow_material.get_editor_property('material_domain')==u.MaterialDomain.MD_VOLUME
assert flow_material.get_editor_property('blend_mode')==u.BlendMode.BLEND_ADDITIVE
assert flow_material.get_editor_property('used_with_heterogeneous_volumes')
report['turbulent_flow_frames']=flow.get_num_frames()
extinction=u.MaterialEditingLibrary.get_material_property_input_node(vapor,u.MaterialProperty.MP_SUBSURFACE_COLOR)
assert extinction and extinction.get_editor_property('description')=='Vapor extinction / inverse centimetres','Vapor extinction is not connected to the RGB volume output'
opts=u.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True)
registry_header=(PROJECT_ROOT/'Source/SuperHeavySim/Public/Recovery/Shared/RecoveryAssets.h').read_text(encoding='utf-8')
runtime=[p.split('.')[0] for p in re.findall(r'TEXT\("(/Game/[^"\n]+)"\)',registry_header)]
pending=[root+'/Maps/L_RecoveryLab']+runtime;seen=set();missing=[]
while pending:
    p=pending.pop()
    if p in seen or not p.startswith('/Game/'):continue
    seen.add(p)
    if not A.does_asset_exist(p):missing.append(p);continue
    pending.extend(str(d) for d in (R.get_dependencies(p,opts) or []))
assert not missing,missing
report['dependencies']=sorted(seen)
print('RUNTIME_DEPENDENCY_COUNT',len(seen))
assert root+'/Meshes/Earth/SM_EarthGlobe' in seen,'Earth dependency missing from the map graph'
assert not any(p.startswith(('/Game/Recovery/','/Game/SuperHeavy/','/Game/Archive/')) for p in seen),[p for p in seen if p.startswith('/Game/Archive/')]
report['success']=True
(PROJECT_ROOT/'Saved/Recovery/experience-asset-audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('EXPERIENCE_ASSET_AUDIT_PASS',len(seen),'resolved runtime dependencies')

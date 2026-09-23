"""Import current render artwork; flight/contact primitives remain authoritative."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
from unreal_materials import u, prop
from unreal_imports import import_scenery_mesh

root='/Game/Starbase'
mapping={'Steel':'M_StainlessFlight','Tiles':'M_StarshipHeatShield',
         'Mechanisms':'M_FlightMechanisms','Nozzles':'M_FlightNozzles'}
for folder,name in (('VehicleDetails','SM_StarshipDetailed'),('VehicleDetails','SM_RCSBlock'),
                    ('Flight','SM_ExhaustEnvelope'),('Flight','SM_LaunchMount')):
    mesh=import_scenery_mesh(ART_ROOT/folder/(name+'.fbx'),root+'/Meshes/'+name)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        chosen=next((v for k,v in mapping.items() if k in str(slot.material_slot_name)),'M_StainlessFlight')
        if name=='SM_ExhaustEnvelope':chosen='M_RaptorPlume'
        if name=='SM_LaunchMount':chosen='M_Graphite'
        mesh.set_material(i,u.load_asset(root+'/Materials/'+chosen))
    settings=mesh.get_editor_property('nanite_settings')
    prop(settings,'enabled',name!='SM_ExhaustEnvelope');prop(settings,'position_precision',2)
    prop(mesh,'nanite_settings',settings)
    editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    editor.remove_collisions(mesh)
    body=mesh.get_editor_property('body_setup')
    if body:prop(body,'collision_trace_flag',u.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
print('VEHICLE_ART_IMPORTED',flush=True)

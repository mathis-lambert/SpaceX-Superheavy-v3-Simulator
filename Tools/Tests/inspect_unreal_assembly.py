
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
import unreal as u, json, os
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
a=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.load_class(None,'/Game/Starbase/Vehicle/Blueprints/BP_SuperHeavy.BP_SuperHeavy_C'),u.Vector(0,0,3544))
a.set_actor_scale3d(u.Vector(22.5,22.5,80))
out=[]
for c in a.get_components_by_class(u.SceneComponent):
    v=c.get_world_location();r=c.get_world_rotation();s=c.get_world_scale()
    item=dict(name=c.get_name(),type=c.get_class().get_name(),loc=[v.x,v.y,v.z],rot=[r.pitch,r.yaw,r.roll],scale=[s.x,s.y,s.z])
    if isinstance(c,u.PrimitiveComponent):
        origin,extent,radius=u.SystemLibrary.get_component_bounds(c)
        item['bounds_center']=[origin.x,origin.y,origin.z];item['bounds_extent']=[extent.x,extent.y,extent.z]
    if isinstance(c,u.ChildActorComponent):
        child=c.get_editor_property('child_actor')
        if child:item['child_components']=[dict(name(x.get_name()),type=x.get_class().get_name()) for x in []]
        if child:
            item['child_components']=[]
            for x in child.get_components_by_class(u.SceneComponent):
                v2=x.get_world_location()
                item['child_components'].append(dict(name=x.get_name(),type=x.get_class().get_name(),loc=[v2.x,v2.y,v2.z]))
    out.append(item)
path=os.path.join(u.Paths.project_saved_dir(),'Recovery','assembly-audit.json')
open(path,'w').write(json.dumps(out,indent=2))
print('ASSEMBLY_AUDIT_READY')

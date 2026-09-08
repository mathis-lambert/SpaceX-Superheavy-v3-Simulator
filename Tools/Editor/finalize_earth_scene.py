
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
import unreal as u
from pathlib import Path
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
levels.load_level('/Game/Starbase/Maps/L_RecoveryLab')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label().startswith('Recovery_Earth_') and isinstance(a,u.StaticMeshActor):
        a.static_mesh_component.set_collision_profile_name('NoCollision',False)
        a.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
levels.save_current_level()
exec(compile((Path(u.Paths.project_dir())/'Tools'/'Tests'/'audit_earth_assets.py').read_text(encoding='utf-8'),'audit_earth_assets.py','exec'))
exec(compile((Path(u.Paths.project_dir())/'Tools'/'Tests'/'audit_recovery_assets.py').read_text(encoding='utf-8'),'audit_recovery_assets.py','exec'))

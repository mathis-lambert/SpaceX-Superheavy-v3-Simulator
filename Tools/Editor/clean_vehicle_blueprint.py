"""Remove obsolete vehicle input entry points; retain external actuator events."""
import unreal as u,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import SAVED_ROOT
path='/Game/Starbase/Vehicle/Blueprints/BP_SuperHeavy'
bp=u.load_asset(path);assert bp
count=u.RecoveryAssetMaintenance.remove_legacy_input_events(bp)
u.BlueprintEditorLibrary.remove_unused_nodes(bp)
assert u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
(SAVED_ROOT/'vehicle-input-cleanup.json').write_text(json.dumps(dict(success=True,removed_input_events=count,asset=path),indent=2))
print('VEHICLE_INPUT_CLEANUP',count)

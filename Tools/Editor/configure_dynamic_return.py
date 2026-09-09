"""Update the authored mission's return guidance; preserve geometry and hardware."""
import unreal as u
from pathlib import Path
import shutil

root = Path(u.Paths.project_dir()).resolve()
source = root / 'Content/Starbase/Data/DA_RecoveryMission.uasset'
backup = root / 'Saved/Recovery/BeforeDynamicReturn/DA_RecoveryMission.uasset'
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(source, backup)
profile = u.load_asset('/Game/Starbase/Data/DA_RecoveryMission')
defaults = u.get_default_object(u.SuperHeavyRecoveryProfile)
for name in ('front_return_offset_m', 'landing_wind_lead_s', 'landing_deceleration_mps2', 'landing_burn_margin_m', 'max_tilt_deg'):
    profile.set_editor_property(name, defaults.get_editor_property(name))
u.EditorAssetLibrary.save_loaded_asset(profile)
u.log('DYNAMIC_RETURN_PROFILE_SAVED')

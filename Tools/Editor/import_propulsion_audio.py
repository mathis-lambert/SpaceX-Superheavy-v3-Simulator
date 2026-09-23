"""Import only the new authored audio, leaving world assets untouched."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
import unreal as u
for name in ['EngineRoar','CoastalWind','EngineRumble','EngineCrackle','CryogenicHiss','Deluge','TowerDrive','TowerContact','MountRelease']:
    task=u.AssetImportTask();task.filename=str(ART_ROOT/'Audio'/(name+'.wav'))
    task.destination_path='/Game/Starbase/Audio';task.destination_name='S_'+name
    task.automated=True;task.replace_existing=True;task.save=False
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=u.load_asset('/Game/Starbase/Audio/S_'+name);assert sound
    sound.set_editor_property('looping',name not in ['TowerContact','MountRelease'])
    sound.set_editor_property('volume',1.)
    sound.set_editor_property('virtualization_mode',u.VirtualizationMode.PLAY_WHEN_SILENT)
    sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE)
    u.EditorAssetLibrary.save_loaded_asset(sound,False)
print('PROPULSION_AUDIO_READY')

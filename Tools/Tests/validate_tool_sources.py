"""Check Python syntax and referenced import sources without running authoring jobs."""
from pathlib import Path
import ast,json
P=Path(__file__).resolve().parents[2]
files=list((P/'Tools').rglob('*.py'))
for file in files:ast.parse(file.read_text(encoding='utf-8-sig'),filename=str(file))
assert not (P/'Scripts').exists()
for folder in ['Earth','Flight','VehicleDetails','Audio','Originals']:assert (P.parent/'ArtSource'/folder).is_dir(),folder
for file in (P/'Source/SuperHeavySim').rglob('*'):
    if file.suffix not in ['.cpp','.h']:continue
    t=file.read_text(encoding='utf-8-sig')
    assert '/Game/Recovery/' not in t and '/Game/SuperHeavy/' not in t,file
print(json.dumps({'success':True,'python_files':len(files),'legacy_runtime_asset_paths':0}))

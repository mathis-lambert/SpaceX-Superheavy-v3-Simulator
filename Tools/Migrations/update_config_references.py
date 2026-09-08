"""Update native settings references ahead of reference-aware content migration."""
from pathlib import Path
P=Path(__file__).resolve().parents[2]
mapping={'/Game/Recovery':'/Game/Starbase','/Game/SuperHeavy':'/Game/Starbase/Vehicle',
 '/Game/SimBlank':'/Game/Archive/SimulationTemplate','/Game/Maps':'/Game/Archive/PrototypeMaps',
 '/Game/WaterMaterials':'/Game/ThirdParty/WaterMaterials','/Game/MWLandscapeAutoMaterial':'/Game/ThirdParty/MWLandscapeAutoMaterial'}
for root in [P/'Config',P/'Saved/Config']:
    for f in root.rglob('*.ini'):
        text=f.read_text(encoding='utf-8-sig');updated=text
        for old,new in mapping.items():updated=updated.replace(old,new)
        if updated!=text:
            if root==P/'Saved/Config':
                backup=P/'Saved/Recovery/BeforeRestructure'/f.relative_to(P)
                backup.parent.mkdir(parents=True,exist_ok=True)
                if not backup.exists():backup.write_bytes(f.read_bytes())
            f.write_text(updated,encoding='utf-8');print('UPDATED',f.relative_to(P))

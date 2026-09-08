"""One-time verified filesystem migration. Never moves raw Unreal packages."""
from pathlib import Path
import re,json,shutil
P=Path(__file__).resolve().parents[1];W=P.parent
assert (P/'Saved/Recovery/BeforeRestructure/manifest.json').exists()
manifest=[]
def move(src,dst):
    src,dst=src.resolve(),dst.resolve()
    assert src.is_relative_to(W) and dst.is_relative_to(W)
    if not src.exists() and dst.exists():
        manifest.append([str(src.relative_to(W)),str(dst.relative_to(W))]);return
    assert src.exists() and not dst.exists(),(src,dst)
    dst.parent.mkdir(parents=True,exist_ok=True)
    shutil.move(str(src),str(dst));manifest.append([str(src.relative_to(W)),str(dst.relative_to(W))])

# The original sources remain available; their locations describe their purpose.
for old,new in {'3d':'Originals','earth':'Earth','recovery':'Flight','overhaul':'VehicleDetails','audio':'Audio'}.items():
    move(W/'assets'/old,W/'ArtSource'/new)

source=P/'Source/SuperHeavySim';includes={}
for scope in ['Public','Private']:
    folder=source/scope/'Recovery'
    for original in list((P/'Saved/Recovery/BeforeRestructure/Source/SuperHeavySim'/scope/'Recovery').iterdir()):
        f=folder/original.name
        if original.is_dir():continue
        n=f.stem
        category=('Tests' if 'Audit' in n else 'Interface' if any(v in n for v in ['HUD','Menu','PlayerController']) else
                  'Presentation' if any(v in n for v in ['Presentation','Cameras','Sky','EnvironmentProfile','Audio','Vapor','Lighting']) else 'Flight')
        dst=folder/category/f.name
        if f.suffix=='.h':includes['Recovery/'+f.name]='Recovery/'+category+'/'+f.name
        move(f,dst)

prefixes={'/Game/Starbase':'/Game/Starbase','/Game/Starbase/Vehicle':'/Game/Starbase/Vehicle',
 '/Game/ThirdParty/MWLandscapeAutoMaterial':'/Game/ThirdParty/MWLandscapeAutoMaterial','/Game/ThirdParty/WaterMaterials':'/Game/ThirdParty/WaterMaterials',
 '/Game/Archive/SimulationTemplate':'/Game/Archive/SimulationTemplate','/Game/Archive/PrototypeMaps':'/Game/Archive/PrototypeMaps'}
def paths(text):
    for a,b in prefixes.items():text=text.replace(a,b)
    for a,b in includes.items():text=text.replace(a,b)
    text=text.replace('"Recovery/Interface/RecoveryMenu.h"','"Recovery/Interface/RecoveryMenu.h"')
    return text
for f in source.rglob('*'):
    if f.suffix not in ['.h','.cpp']:continue
    text=f.read_text(encoding='utf-8-sig')
    if 'RecoveryAssets::' in text:
        originals=list((P/'Saved/Recovery/BeforeRestructure/Source').rglob(f.name))
        assert len(originals)==1,f.name
        text=originals[0].read_text(encoding='utf-8-sig')
    f.write_text(paths(text),encoding='utf-8')
# Central registry for runtime asset references (engine built-ins remain engine paths).
assets={}
for f in source.rglob('*.cpp'):
    t=f.read_text();changed=False
    for path in re.findall(r'TEXT\("(/Game/[^"\n]+)"\)',t):
        if path not in assets:
            name=re.sub(r'[^a-zA-Z0-9_]','_',path.rsplit('/',1)[-1].split('.')[-1])
            if name in assets.values():name+='_'+str(len(assets))
            assets[path]=name
        t=t.replace('TEXT("'+path+'")','RecoveryAssets::'+assets[path]);changed=True
    if changed:f.write_text('#include "Recovery/Shared/RecoveryAssets.h"\n'+t,encoding='utf-8')
header='#pragma once\n#include "CoreMinimal.h"\n\n// Canonical runtime references; update here when moving authored content.\nnamespace RecoveryAssets\n{\n'
header+=''.join('    inline constexpr TCHAR '+name+'[]=TEXT("'+path+'");\n' for path,name in assets.items())+'}\n'
(source/'Public/Recovery/Shared/RecoveryAssets.h').write_text(header,encoding='utf-8')

# UI palette/widgets styles have one implementation shared with future panels.
menu=source/'Private/Recovery/Interface/RecoveryMenu.cpp';t=menu.read_text()
start=t.index('namespace RecoveryUI');end=t.index('TSharedRef<SWidget>',start)
styles=t[start:end];decl='''#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
namespace RecoveryUI
{
    extern const FLinearColor Accent, Muted, Ink;
    const FButtonStyle& ButtonStyle();
    const FButtonStyle& PrimaryStyle();
    const FSliderStyle& DaylightSliderStyle();
}
'''
(source/'Public/Recovery/Shared/RecoveryUIStyle.h').write_text(decl)
(source/'Private/Recovery/Shared').mkdir(exist_ok=True)
(source/'Private/Recovery/Shared/RecoveryUIStyle.cpp').write_text('#include "Recovery/Shared/RecoveryUIStyle.h"\n#include "Brushes/SlateColorBrush.h"\n#include "Brushes/SlateRoundedBoxBrush.h"\n'+styles)
menu.write_text('#include "Recovery/Shared/RecoveryUIStyle.h"\n'+t[:start]+t[end:])

tools=P/'Tools';shared=tools/'Shared';shared.mkdir(parents=True,exist_ok=True)
(shared/'project_paths.py').write_text('''"""Canonical source, output, and content roots for standalone authoring tools."""
from pathlib import Path
PROJECT_ROOT=Path(__file__).resolve().parents[2]
ART_ROOT=PROJECT_ROOT.parent/'ArtSource'
CONTENT_ROOT='/Game/Starbase'
SAVED_ROOT=PROJECT_ROOT/'Saved'/'Recovery'
''')
archive={'refactor_experience_sources','translate_and_nest_ui','restructure_sources','restructure_content','backup_restructure','migrate_flight_profile_v2'}
art={'build_tower_meshes','build_flight_art','build_earth_art','build_overhaul_art','bake_vapor_atlas','prepare_scene_audio'}
data={'fetch_earth_data','fetch_overhaul_textures','prepare_earth_water_mask','prepare_earth_scenery'}
for f in list((P/'Scripts').iterdir()):
    if not f.is_file():continue
    n=f.stem
    group='Migrations' if n in archive else 'Art' if n in art else 'Data' if n in data else 'Shared' if n=='earth_geography' else 'Tests' if n.startswith(('test_','audit_','inspect_','probe_','collect_','analyze_')) else 'Runtime' if n=='launch_recovery' else 'Editor'
    dst=tools/group/f.name;move(f,dst)
    if dst.suffix not in ['.py','.ps1','.json']:continue
    t=paths(dst.read_text(encoding='utf-8-sig'))
    if dst.suffix=='.py' and group!='Migrations':
        # Every executable lives one directory beneath Tools and shares the same imports.
        preamble='import sys\nfrom pathlib import Path\nsys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))\nfrom project_paths import PROJECT_ROOT, ART_ROOT\n'
        # Keep original module docstrings at the top.
        pos=0
        if t.startswith('"""'):pos=t.index('"""',3)+3
        t=t[:pos]+'\n'+preamble+t[pos:]
        for pattern in ['Path(__file__).resolve().parents[1]','Path(__file__).resolve().parents[2]']:
            # Do not alter the bootstrap's Tools/Shared path.
            t=t.replace(pattern+"/'assets'",'ART_ROOT').replace(pattern+" / 'assets'",'ART_ROOT')
        t=t.replace("Path(__file__).resolve().parents[1];",'PROJECT_ROOT;').replace("Path(__file__).resolve().parents[1]\n",'PROJECT_ROOT\n')
        t=t.replace("Path(u.Paths.project_dir()).resolve().parent/'assets'",'ART_ROOT').replace("root.parent/'assets/audio'","ART_ROOT/'Audio'")
        t=t.replace("ROOT.parent/'assets/audio'","ART_ROOT/'Audio'").replace("root.parent/'assets/overhaul/sources.json'","ART_ROOT/'VehicleDetails/sources.json'")
        t=re.sub(r"os\.path\.abspath\(os\.path\.join\(os\.path\.dirname\(__file__\),\s*'\.\.',\s*'\.\.',\s*'assets',\s*'recovery'\)\)","str(ART_ROOT/'Flight')",t)
        t=t.replace("os.path.abspath(os.path.join(u.Paths.project_dir(),'..','assets','recovery'))","str(ART_ROOT/'Flight')")
        for a,b in {'earth':'Earth','recovery':'Flight','overhaul':'VehicleDetails','audio':'Audio'}.items():
            t=t.replace("ART_ROOT/'"+a+"'","ART_ROOT/'"+b+"'").replace("ART_ROOT / '"+a+"'","ART_ROOT / '"+b+"'")
        for a,b in manifest:
            if 'Source/' in a:t=t.replace(a.split('SuperHeavySim/',1)[-1],b.split('SuperHeavySim/',1)[-1])
    if dst.suffix=='.ps1':t=t.replace("Join-Path $PSScriptRoot '..'","Join-Path $PSScriptRoot '../..'")
    dst.write_text(t,encoding='utf-8')
move(P/'Scripts/lib/unreal_materials.py',shared/'unreal_materials.py')
f=tools/'Editor/build_experience_assets.py';f.write_text(f.read_text().replace('from lib.unreal_materials import','from unreal_materials import'))
# Config remains intact, including platform settings. Remove only exact duplicate lines.
f=P/'Config/DefaultEngine.ini';t=paths(f.read_text(encoding='utf-8-sig'))
lines=t.splitlines();seen=set();out=[]
for line in lines:
    if line.startswith('['):seen.clear()
    if '=' in line and not line.startswith(('+','-',';')):
        if line in seen:continue
        seen.add(line)
    out.append(line)
f.write_text('\n'.join(out)+'\n',encoding='utf-8')
# Historical reports stay explicitly archived, not mixed with current guides.
for f in list((P/'Docs').iterdir()):
    if f.is_file():move(f,P/'Docs/Archive'/f.name)
(P/'Saved/Recovery/filesystem-migration.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('FILESYSTEM_RESTRUCTURE_READY',len(manifest),'moves',len(assets),'centralized asset paths')

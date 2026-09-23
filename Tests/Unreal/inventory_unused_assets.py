"""Read-only package reachability report; includes code loads and soft references.

This does not delete packages. Removal is performed offline from this manifest,
with a verified recovery copy, then checked by a fresh editor dependency audit.
"""
import sys,re,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'Tools'/'Shared'))
from project_paths import PROJECT_ROOT,SAVED_ROOT
import unreal as u
A=u.EditorAssetLibrary;R=u.AssetRegistryHelpers.get_asset_registry();R.search_all_assets(True)
all_packages={str(d.package_name) for d in R.get_assets_by_path('/Game',True)}
roots=set();source_paths=[]
for base in ('Source','Config'):
    for path in (PROJECT_ROOT/base).rglob('*'):
        if path.suffix not in ('.h','.cpp','.ini') or 'Tests' in path.parts:continue
        text=path.read_text(encoding='utf-8',errors='replace')
        # Cook inclusion/exclusion policies do not prove runtime use. Treating
        # these folder names as roots previously made every asset "reachable",
        # even assets explicitly excluded from cooking.
        if path.suffix=='.ini':
            text='\n'.join(line for line in text.splitlines()
                           if not line.lstrip().startswith(';')
                           and 'DirectoriesToAlwaysCook' not in line
                           and 'DirectoriesToNeverCook' not in line)
        for match in re.findall(r'/Game/[A-Za-z0-9_./]+',text):
            package=match.split('.')[0].rstrip('/')
            if package in all_packages:roots.add(package)
            else:roots.update(p for p in all_packages if p.startswith(package+'/'))
            source_paths.append(dict(file=str(path.relative_to(PROJECT_ROOT)),path=package))
# Preserve user levels; vendor demonstration maps and archived prototypes are
# not runtime roots. Their actually-used textures/meshes remain dependencies.
for d in R.get_assets_by_path('/Game',True):
    p=str(d.package_name)
    if str(d.asset_class_path.asset_name)=='World' and not p.startswith(('/Game/Archive/','/Game/ThirdParty/')):roots.add(p)
options=u.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,
    include_searchable_names=True,include_soft_management_references=True,include_hard_management_references=True)
pending=list(roots);reachable=set()
while pending:
    p=pending.pop()
    if p in reachable or not p.startswith('/Game/'):continue
    reachable.add(p);pending.extend(str(v) for v in (R.get_dependencies(p,options) or []))
unused=all_packages-reachable
external=[]
for p in sorted(unused):
    refs=[str(x) for x in R.get_referencers(p,options) or [] if str(x) not in unused]
    if refs:external.append(dict(package=p,referencers=refs))
assert not external,external
files=[]
for p in sorted(unused):
    base=PROJECT_ROOT/'Content'/p[len('/Game/'):]
    for ext in ('.uasset','.umap','.uexp','.ubulk'):
        path=base.with_suffix(ext)
        if path.is_file():files.append(dict(path=str(path.relative_to(PROJECT_ROOT)),bytes=path.stat().st_size,package=p))
report=dict(success=True,roots=sorted(roots),reachable=sorted(reachable),unused=sorted(unused),files=files,
    bytes=sum(f['bytes'] for f in files),code_loads=source_paths,external_referencers=external)
(SAVED_ROOT/'unused-asset-inventory.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('UNUSED_INVENTORY',len(unused),'packages',report['bytes'],'bytes',len(reachable),'retained')

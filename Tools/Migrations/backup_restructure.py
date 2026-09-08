"""Create a verified, immutable local recovery point before asset renames."""
from pathlib import Path
import hashlib,json,shutil
root=Path(__file__).resolve().parents[1]
dst=root/'Saved/Recovery/BeforeRestructure'
manifest=[]
for folder in ['Content','Source','Scripts','Config','Docs']:
    for p in (root/folder).rglob('*'):
        if not p.is_file() or '__pycache__' in p.parts:continue
        out=dst/p.relative_to(root);out.parent.mkdir(parents=True,exist_ok=True)
        assert not out.exists(),f'Backup already exists: {out}'
        shutil.copy2(p,out)
        digest=lambda f:hashlib.file_digest(f.open('rb'),'sha256').hexdigest()
        a,b=digest(p),digest(out);assert a==b,str(p)
        manifest.append({'path':p.relative_to(root).as_posix(),'bytes':p.stat().st_size,'sha256':a})
(dst/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps({'verified_files':len(manifest),'bytes':sum(x['bytes'] for x in manifest)}))

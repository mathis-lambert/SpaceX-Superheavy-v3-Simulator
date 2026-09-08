"""Install the official, version-pinned DLSS package locally without editing vendor code."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import sys
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import PROJECT_ROOT, ART_ROOT, SAVED_ROOT

ARCHIVE=ART_ROOT/'ThirdParty/NVIDIA/UE5.8_DLSS4.5Plugin_v8.7.2.zip'
DEST=PROJECT_ROOT/'Plugins/NVIDIA'
PLUGINS={'DLSS','StreamlineNGXCommon'}

def main():
    DEST.mkdir(parents=True,exist_ok=True)
    manifest=[]
    with zipfile.ZipFile(ARCHIVE) as package:
        for entry in package.infolist():
            name=PurePosixPath(entry.filename)
            if len(name.parts)<3 or name.parts[0]!='Plugins' or name.parts[1] not in PLUGINS or entry.is_dir():
                continue
            relative=Path(*name.parts[1:])
            target=(DEST/relative).resolve()
            assert target.is_relative_to(DEST.resolve()),'Unsafe archive path'
            data=package.read(entry)
            if target.exists():
                assert target.read_bytes()==data,f'Local vendor changes found: {relative}'
            else:
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(data)
            manifest.append(dict(path=str(relative),sha256=hashlib.sha256(data).hexdigest()))
    project=PROJECT_ROOT/'SuperHeavySim.uproject'
    backup=SAVED_ROOT/'BeforeStrictPhysics/SuperHeavySim.uproject'
    if not backup.exists():shutil.copy2(project,backup)
    descriptor=json.loads(project.read_text(encoding='utf-8-sig'))
    if not any(p['Name']=='DLSS' for p in descriptor['Plugins']):
        descriptor['Plugins'].append(dict(Name='DLSS',Enabled=True,Optional=True,PlatformAllowList=['Win64']))
    project.write_text(json.dumps(descriptor,indent=2)+'\n',encoding='utf-8')
    (DEST/'provenance.json').write_text(json.dumps(dict(source='https://developer.nvidia.com/rtx/dlss',archive=ARCHIVE.name,archive_sha256=hashlib.sha256(ARCHIVE.read_bytes()).hexdigest(),version='8.7.2-NGX310.6.0',engine='5.8',files=manifest),indent=2),encoding='utf-8')
    print('DLSS_LOCAL_PACKAGE_INSTALLED',len(manifest),'files')

if __name__=='__main__':main()

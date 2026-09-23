"""Publish a verified alpha archive; release.json is the completion marker."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
from urllib.parse import quote, urlsplit


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def release_files(directory, repository, tag, commit, base_url):
    if not re.fullmatch(r'[A-Za-z0-9_.-]+', repository):
        raise ValueError('Invalid repository name')
    if not re.fullmatch(r'v\d+\.\d+\.\d+-alpha\.\d+', tag):
        raise ValueError('Expected an alpha version tag')
    if not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('Expected a full source commit')
    url = urlsplit(base_url)
    if url.scheme != 'https' or not url.netloc or url.username or url.query or url.fragment:
        raise ValueError('Public base URL must be a plain HTTPS URL')
    stem = f'Starbase-{tag[1:]}'
    names = [f'{stem}.zip', f'{stem}.zip.sha256', f'{stem}-artifacts.json']
    if {p.name for p in directory.iterdir()} != set(names):
        raise ValueError('Release directory must contain only the three validated artifacts')
    if any((directory / name).is_symlink() or not (directory / name).is_file() for name in names):
        raise ValueError('Release artifacts must be regular files')
    evidence = json.loads((directory / names[2]).read_text(encoding='utf-8'))
    archive_hash = digest(directory / names[0])
    if (evidence['version'] != tag[1:] or evidence['source_commit'] != commit
            or evidence['archive'] != names[0] or evidence['archive_sha256'] != archive_hash
            or evidence['archive_bytes'] != (directory / names[0]).stat().st_size
            or evidence.get('zip_crc_verified') is not True):
        raise ValueError('Archive provenance or checksum mismatch')
    if (directory / names[1]).read_text(encoding='utf-8').strip() != f'{archive_hash}  {names[0]}':
        raise ValueError('Checksum file mismatch')
    prefix = f'{repository}/{tag}/'
    files = [{
        'name': name, 'bytes': (directory / name).stat().st_size,
        'sha256': digest(directory / name),
        'url': f'{base_url.rstrip("/")}/{quote(prefix + name, safe="/")}',
    } for name in names]
    gpu_validation = evidence.get('gpu_validation')
    if gpu_validation not in ('passed', 'not_run'):
        raise ValueError('Missing or invalid GPU validation status')
    return {'schema_version': 1, 'repository': repository, 'tag': tag,
            'source_commit': commit, 'platform': 'Windows x64',
            'gpu_validation': gpu_validation, 'files': files}


class S3:
    def __init__(self, endpoint, bucket):
        if urlsplit(endpoint).scheme != 'https' or not urlsplit(endpoint).netloc:
            raise ValueError('S3 endpoint must use HTTPS')
        if not re.fullmatch(r'[a-z0-9][a-z0-9.-]{1,61}[a-z0-9]', bucket):
            raise ValueError('Invalid S3 bucket')
        self.endpoint, self.bucket = endpoint, bucket

    def run(self, *args):
        result = subprocess.run(['aws', '--endpoint-url', self.endpoint, '--no-cli-pager',
                                 *args], check=True, capture_output=True, text=True)
        return json.loads(result.stdout) if result.stdout.strip() else {}

    def list(self, prefix):
        return {o['Key'] for o in self.run('s3api', 'list-objects-v2', '--bucket',
                self.bucket, '--prefix', prefix).get('Contents', [])}

    def head(self, key):
        return self.run('s3api', 'head-object', '--bucket', self.bucket, '--key', key)

    def put(self, path, key, checksum):
        self.run('s3', 'cp', str(path), f's3://{self.bucket}/{key}', '--no-progress',
                 '--only-show-errors', '--metadata', f'sha256={checksum}',
                 '--cache-control', 'public,max-age=31536000,immutable')


def publish(directory, manifest, store):
    prefix = f'{manifest["repository"]}/{manifest["tag"]}/'
    with tempfile.TemporaryDirectory() as temp:
        marker = Path(temp) / 'release.json'
        marker.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
        entries = [(directory / item['name'], item) for item in manifest['files']]
        entries.append((marker, {'name': marker.name, 'bytes': marker.stat().st_size,
                                 'sha256': digest(marker)}))
        existing = store.list(prefix)
        expected = {prefix + item['name'] for _, item in entries}
        if existing - expected:
            raise ValueError('Unexpected existing objects: refusing to overwrite this release')
        # Check every existing object before uploading anything. Retry only identical partial releases.
        for _, item in entries:
            key = prefix + item['name']
            if key in existing:
                head = store.head(key)
                if head['ContentLength'] != item['bytes'] or head.get('Metadata', {}).get('sha256') != item['sha256']:
                    raise ValueError(f'Existing release differs: {key}')
        if prefix + marker.name in existing and existing != expected:
            raise ValueError('Published release is incomplete; refusing to modify it')
        for path, item in entries:
            key = prefix + item['name']
            if key not in existing:
                store.put(path, key, item['sha256'])
            head = store.head(key)
            if head['ContentLength'] != item['bytes'] or head.get('Metadata', {}).get('sha256') != item['sha256']:
                raise ValueError(f'Remote verification failed: {key}')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--commit', required=True)
    args = parser.parse_args()
    manifest = release_files(args.directory, args.repository, args.tag, args.commit,
                             os.environ['RELEASES_PUBLIC_BASE_URL'])
    store = S3(os.environ['RELEASES_S3_ENDPOINT'], os.environ['RELEASES_S3_BUCKET'])
    publish(args.directory, manifest, store)
    print(json.dumps(manifest, indent=2))
    if summary := os.environ.get('GITHUB_STEP_SUMMARY'):
        with open(summary, 'a', encoding='utf-8') as stream:
            stream.write(f'## {args.tag}\n\n')
            for item in manifest['files']:
                stream.write(f'- [{item["name"]}]({item["url"]}) — {item["bytes"]} bytes\n')


if __name__ == '__main__':
    main()

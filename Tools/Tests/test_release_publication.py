"""Release provenance, immutable publication and interrupted-upload recovery."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Runtime'))
from publish_release import publish, release_files


class Store:
    def __init__(self):
        self.objects = {}
        self.uploads = []
        self.fail_at = None

    def list(self, prefix):
        return {key for key in self.objects if key.startswith(prefix)}

    def head(self, key):
        return self.objects[key]

    def put(self, path, key, checksum):
        if len(self.uploads) == self.fail_at:
            raise OSError('Interrupted upload')
        self.uploads.append(key)
        self.objects[key] = {'ContentLength': path.stat().st_size,
                             'Metadata': {'sha256': checksum}}


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.stem = 'Starbase-0.1.0-alpha.12'
        self.name = self.stem + '.zip'
        data = b'fixture archive'
        checksum = hashlib.sha256(data).hexdigest()
        (self.directory / self.name).write_bytes(data)
        (self.directory / (self.name + '.sha256')).write_text(f'{checksum}  {self.name}\n')
        (self.directory / (self.stem + '-artifacts.json')).write_text(json.dumps({
            'version': '0.1.0-alpha.12', 'source_commit': 'a' * 40,
            'archive': self.name, 'archive_sha256': checksum,
            'archive_bytes': len(data), 'zip_crc_verified': True,
        }))

    def manifest(self, **kwargs):
        return release_files(self.directory, **dict({
            'repository': 'SpaceX-Superheavy-v3-Simulator',
            'tag': 'v0.1.0-alpha.12', 'commit': 'a' * 40,
            'base_url': 'https://cdn.example.com/software-releases',
        }, **kwargs))

    def test_urls_and_idempotent_publication(self):
        manifest = self.manifest()
        self.assertIn('/SpaceX-Superheavy-v3-Simulator/v0.1.0-alpha.12/', manifest['files'][0]['url'])
        store = Store()
        publish(self.directory, manifest, store)
        self.assertTrue(store.uploads[-1].endswith('/release.json'))
        publish(self.directory, manifest, store)
        self.assertEqual(len(store.uploads), 4)

    def test_wrong_tag_commit_checksum_and_extra_files_fail(self):
        for kwargs in ({'tag': '../../x'}, {'commit': 'b' * 40}, {'repository': '../x'}):
            with self.assertRaises(ValueError):
                self.manifest(**kwargs)
        (self.directory / self.name).write_bytes(b'tampered')
        with self.assertRaises(ValueError):
            self.manifest()
        (self.directory / '.env').write_text('fixture')
        with self.assertRaises(ValueError):
            self.manifest()

    def test_interrupted_upload_has_no_completion_marker_and_resumes(self):
        manifest, store = self.manifest(), Store()
        store.fail_at = 1
        with self.assertRaises(OSError):
            publish(self.directory, manifest, store)
        self.assertFalse(any(key.endswith('/release.json') for key in store.objects))
        store.fail_at = None
        publish(self.directory, manifest, store)
        self.assertEqual(len(store.uploads), 4)

    def test_conflicting_release_is_never_overwritten(self):
        manifest, store = self.manifest(), Store()
        publish(self.directory, manifest, store)
        changed = copy.deepcopy(manifest)
        changed['files'][0]['sha256'] = 'b' * 64
        with self.assertRaises(ValueError):
            publish(self.directory, changed, store)
        self.assertEqual(len(store.uploads), 4)

    def test_missing_object_in_completed_release_is_not_silently_repaired(self):
        manifest, store = self.manifest(), Store()
        publish(self.directory, manifest, store)
        del store.objects[store.uploads[0]]
        with self.assertRaises(ValueError):
            publish(self.directory, manifest, store)
        self.assertEqual(len(store.uploads), 4)


if __name__ == '__main__':
    unittest.main()

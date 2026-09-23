"""Package integrity and optional, exact-package local GPU evidence."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from Tools.Runtime import archive_alpha


class ArchiveValidationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.release = self.root / 'Starbase-0.1.0-alpha.1'
        files = []
        for name in ('Windows/SuperHeavySim.exe',
                     'Windows/SuperHeavySim/Binaries/Win64/SuperHeavySim.exe'):
            path = self.release / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'package fixture')
            files.append({'path': name, 'sha256': archive_alpha.sha256(path)})
        self.manifest = self.release / 'build-manifest.json'
        self.manifest.write_text(json.dumps({
            'version': '0.1.0-alpha.1', 'source_commit': 'a' * 40, 'files': files,
        }), encoding='utf-8')
        self.validation = {'success': True, 'source_commit': 'a' * 40,
                           'version': '0.1.0-alpha.1',
                           'manifest_sha256': archive_alpha.sha256(self.manifest)}

    def run_archive(self, with_validation=True):
        evidence = self.root / 'validation.json'
        evidence.write_text(json.dumps(self.validation), encoding='utf-8')
        command = [sys.executable, archive_alpha.__file__, str(self.release)]
        if with_validation:
            command += ['--validation', str(evidence)]
        return subprocess.run(command, capture_output=True, text=True)

    def test_ci_package_does_not_claim_gpu_validation(self):
        result = self.run_archive(with_validation=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['gpu_validation'], 'not_run')

    def test_matching_package_is_archived(self):
        result = self.run_archive()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['gpu_validation'], 'passed')
        self.assertTrue((self.root / (self.release.name + '.zip')).exists())

    def test_same_commit_different_package_is_rejected(self):
        self.manifest.write_text(self.manifest.read_text() + '\n', encoding='utf-8')
        result = self.run_archive()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('passing validation', result.stderr)

    def test_failed_or_wrong_version_validation_is_rejected(self):
        for field, value in (('success', False), ('version', '0.1.0-alpha.2'),
                             ('manifest_sha256', None), ('source_commit', 'b' * 40)):
            with self.subTest(field=field):
                original = self.validation[field]
                self.validation[field] = value
                self.assertNotEqual(self.run_archive().returncode, 0)
                self.validation[field] = original

    def test_modified_binary_is_rejected(self):
        (self.release / 'Windows/SuperHeavySim.exe').write_bytes(b'changed')
        result = self.run_archive()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('checksum mismatch', result.stderr)

    def test_ci_still_rejects_modified_binary(self):
        (self.release / 'Windows/SuperHeavySim.exe').write_bytes(b'changed')
        self.assertNotEqual(self.run_archive(with_validation=False).returncode, 0)

    def test_null_local_report_is_not_silently_ignored(self):
        self.validation = None
        self.assertNotEqual(self.run_archive().returncode, 0)

"""Verify package integrity and create its portable ZIP; GPU testing is local."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("release", type=Path)
    parser.add_argument("--validation", type=Path,
                        help="Optional local GPU report bound to this exact package")
    args = parser.parse_args()
    release = args.release.resolve()
    manifest = json.loads((release / "build-manifest.json").read_text(encoding="utf-8-sig"))
    validation = None
    if args.validation:
        validation = json.loads(args.validation.read_text(encoding="utf-8-sig"))
        if not isinstance(validation, dict):
            raise SystemExit("A validation report must be a JSON object")
    if validation is not None and (not validation.get("success")
            or validation["source_commit"] != manifest["source_commit"]
            or validation.get("version") != manifest["version"]
            or validation.get("manifest_sha256") != sha256(release / "build-manifest.json")):
        raise SystemExit("A passing validation of this source build is required")
    expected = {"build-manifest.json"}
    for entry in manifest["files"]:
        path = (release / entry["path"]).resolve()
        if not path.is_relative_to(release) or not path.is_file() or sha256(path) != entry["sha256"]:
            raise SystemExit(f"Release checksum mismatch: {entry['path']}")
        expected.add(entry["path"])
    actual = {p.relative_to(release).as_posix() for p in release.rglob("*") if p.is_file()}
    if actual != expected:
        raise SystemExit(f"Unexpected or missing release files: {sorted(actual ^ expected)}")
    archive = release.parent / (release.name + ".zip")
    if archive.exists():
        raise SystemExit(f"Archive already exists: {archive}")
    with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=1) as output:
        for relative in sorted(expected):
            output.write(release / relative, f"{release.name}/{relative}")
    with zipfile.ZipFile(archive) as output:
        corrupt = output.testzip()
        if corrupt:
            raise SystemExit(f"ZIP verification failed: {corrupt}")
    digest = sha256(archive)
    archive.with_name(archive.name + ".sha256").write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    artifact = {
        "version": manifest["version"],
        "source_commit": manifest["source_commit"],
        "archive": archive.name,
        "archive_bytes": archive.stat().st_size,
        "archive_sha256": digest,
        "manifest_sha256": sha256(release / "build-manifest.json"),
        "game_executable_sha256": sha256(release / "Windows/SuperHeavySim/Binaries/Win64/SuperHeavySim.exe"),
        "files": len(expected),
        "zip_crc_verified": True,
        "gpu_validation": "passed" if validation is not None else "not_run",
    }
    archive.with_name(archive.stem + "-artifacts.json").write_text(json.dumps(artifact, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(artifact, indent=2))


if __name__ == "__main__":
    main()

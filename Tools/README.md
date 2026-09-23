# Project tools

Run PowerShell commands from the repository root. Build and test scripts accept
`-EngineRoot`; the default is `D:/Engines/UE_5.8`. Generated output belongs in
`Saved/` or `Releases/`, never beside source files.

## Build and release

| Entry point | Purpose |
|---|---|
| `Runtime/build_simulator.ps1` | Compile the Unreal editor target |
| `Runtime/launch_recovery.ps1` | Launch the simulation from the editor build |
| `Runtime/package_alpha.ps1` | Cook and package the Windows application |
| `Runtime/build_release.ps1` | Build, validate, package and archive a release |
| `Runtime/archive_alpha.py` | Archive a validated package with checksums |
| `Runtime/publish_release.py` | Publish verified artifacts to S3 |
| `Runtime/install_dlss.py` | Install the official vendor plugin from its distribution |

See [CI and releases](../Docs/CI_RELEASES.md) for runner requirements and publication.
Packaging uses committed assets; it does not regenerate the world or require
the external authoring sources.

## Validation

```powershell
python -m pip install -r Tools/Tests/requirements-ci.txt
python -m unittest discover -s Tools/Tests -p 'test_*.py' -v
./Tools/Tests/test_physics_models.ps1
./Tools/Tests/test_experience.ps1
```

The Python suite runs without Unreal. Physics and experience tests require a
built editor. `test_physical_recovery.ps1` exercises the flight matrix;
`test_contact_fixtures.ps1`, `test_resilience.ps1`,
`test_emergency_recovery.ps1`, `test_ground_sequence.ps1` and
`test_marine_contact.ps1` cover their respective systems.
`test_packaged_alpha.ps1` validates the actual packaged application.

Use `test_experience.ps1 -SkipMatrix -SkipAssets -SkipFlight -MenuAudits RecoveryUIAudit`
for the interface, or `RecoveryEarthAudit` for cameras. These use the same
maintained runner as the complete experience audit.

`measure_experience.ps1`, `measure_clouds.ps1` and `review_clouds.ps1` capture
performance and visual evidence. The `analyze_*.py` tools consume those captures;
`compare_visual_reviews.py` compares images. Asset audits and inspection scripts
run inside Unreal's Python environment. `inventory_unused_assets.py` reports
package reachability, including soft references and code loads; it does not
delete assets.

## Asset authoring

| Directory | Execution environment and responsibility |
|---|---|
| `Editor/` | Unreal Python: import assets, build materials, update authored scenes |
| `Art/` | Blender or Python, as specified in each script: meshes, volume data, audio |
| `Data/` | Python: geographic downloads and preprocessing |
| `Shared/` | Shared paths, coordinates, material graphs and validation helpers |

`Editor/build_visual_renewal.py` is the canonical visual rebuild entry point.
Use [visual renewal](../Docs/VISUAL_RENEWAL.md) for its scope and prerequisites;
individual builders are deliberately retained for targeted reimports. Do not
execute every authoring script as a setup step. The committed map and packages
are authoritative, and authoring may need source data under `../ArtSource`.

Completed migrations and version-specific report collectors have been retired.
Historical evidence under `Docs/Validation` and `Docs/Releases` describes the
revision at which it was captured; paths and hashes in those reports are not
instructions to regenerate the current project.

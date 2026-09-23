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

## Tests and analysis

Test runners and fixtures live in [Tests](../Tests/README.md).
`Analysis/` contains offline CSV, audio and image comparison tools; these do not
launch the simulator or generate assets.

## Asset authoring

| Directory | Execution environment and responsibility |
|---|---|
| `Editor/` | Unreal Python: import assets, build materials, update authored scenes |
| `Art/` | Blender or Python, as specified in each script: meshes, volume data, audio |
| `Data/` | Python: geographic downloads and preprocessing |
| `Shared/` | Shared paths, coordinates and material graphs |

`Editor/build_visual_renewal.py` is the canonical visual rebuild entry point.
Use [asset authoring](../Docs/AUTHORING.md) for its scope and prerequisites;
individual builders are deliberately retained for targeted reimports. Do not
execute every authoring script as a setup step. The committed map and packages
are authoritative, and authoring may need source data under `../ArtSource`.

Completed migrations and version-specific report collectors have been retired.
Keep generated reports and comparison baselines outside the source tree. Pass
`--baseline-prefix` explicitly to `analyze_dynamic_return.py`; old report sets
remain available in Git history. Current validation guidance is in
[Validation](../Docs/VALIDATION.md).

# Windows alpha release validation

Released build: **0.1.0-alpha.2**, 9 September 2026.
Game source commit: `31de1743d31e03644ab9d3eb76a541ed6c233f8d`.
Release tag: `v0.1.0-alpha.2`. The tag also includes the release tooling and
validation records; no flight or rendering source changed after the game build.

The first candidate was retained locally for diagnosis and is superseded by
alpha.2. Use `Releases/Starbase-0.1.0-alpha.2/Windows/SuperHeavySim.exe` or extract
the complete `Releases/Starbase-0.1.0-alpha.2.zip`. Unreal Editor is not required.
The ZIP is 1,191,406,558 bytes (approximately 1.19 GB).

## Final fixes

- Keep editor-only asset scans and authoring toolsets out of the standalone game.
- Package DirectX 12 / SM6 shaders, excluding the unused DirectX 11 shader target.
- Load the NVIDIA runtime in monolithic Windows builds and preserve requested
  reconstruction settings until that runtime is initialized.
- Preserve each package in a new versioned directory, isolate test preferences
  and logs, and verify file hashes before testing and archiving.
- Include the NVIDIA license and correct the Windows runtime installation path.

## Standalone validation

Tests ran against the packaged Windows bootstrap and game executable, using
an isolated user directory on Windows 11, Ryzen 5 9600X, RTX 4070 SUPER.
The game is a Development build to retain the 3D force inspector.

| Check | Result |
|---|---|
| Packaged input and menu audit | 69 checks passed |
| F1/F2/F3, number keys, camera and lab controls | No mission reset |
| J/K before launch and in flight | 0.5x then 1x, passed |
| Pause | Existing physical flight remains frozen |
| Full Crosswind mission at a fixed 15 Hz game step | Passed, 7,256 rendered frames |
| Actual image reconstruction | NVIDIA DLSS Quality, 66.7% internal resolution |
| Peak altitude | 97.257 km |
| Front approach corridor | Verified; minimum mast clearance 11.897 m |
| Physical capture | Both rail supports; engines shut down |
| Eight seconds of unpowered support | 0.0468 m measured drift |
| Chase camera after contact | 121 frames, no discontinuity detected |
| Packaged startup/content errors | No logged errors, ensures or fatal errors in these runs |
| ZIP integrity | Every entry passed CRC verification |

Home, force inspection, approach and secured screenshots were visually inspected.
The preceding physical validation matrix, contact tests, unit tests and limitations
are recorded in [Fixed physics clock](../Validation/FIXED_PHYSICS_CLOCK.md).
The transient playback anomaly observed in the first candidate did not reproduce
in the final isolated audit; no unsupported claim of a separate input fix is made.

Machine-readable reports and exact executable/archive SHA-256 hashes are in
[`0.1.0-alpha.2`](0.1.0-alpha.2). Screenshots and full logs are retained locally in
`Saved/Recovery/Alpha-0.1.0-alpha.2-20260909-113812`.

## Scope and remaining limitations

This release freezes further feature work at the user's request. It does not
complete the original 150-item realism programme. In particular, whole-return
numerical convergence, shorter landing burns, articulated tower mechanics,
terrain/imagery polish and wider hardware qualification remain unfinished.
See [release notes](ALPHA_0.1.0.md). Passing this mission is an alpha regression
check, not evidence of real-flight accuracy or universal hardware compatibility.

## Reproduce

Commit the chosen release version before packaging. The version argument must
match `ProjectVersion`; existing release directories and ZIPs are never replaced.

```powershell
./Tools/Runtime/build_simulator.ps1
./Tools/Runtime/package_alpha.ps1 -Version 0.1.0-alpha.2
./Tools/Tests/test_packaged_alpha.ps1 -Version 0.1.0-alpha.2
python Tools/Runtime/archive_alpha.py Releases/Starbase-0.1.0-alpha.2 --validation <audit-directory>/result.json
```

Generated executables and ZIPs remain outside Git. The release tag versions their
sources, manifests, checksums and validation reports. Nothing was pushed remotely.

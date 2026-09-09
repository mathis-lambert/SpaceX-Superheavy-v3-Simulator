# Alpha.3: site geography and photography

Released build: **0.1.0-alpha.3**, 9 September 2026.
Game source commit: `a7316589023514da3bc2dc136e54716101e379f5`.
Release tag: `v0.1.0-alpha.3`. The tag additionally records these validation
documents; no game source or content changed after packaging.

Launch `Releases/Starbase-0.1.0-alpha.3/Windows/SuperHeavySim.exe`, or extract
the complete `Releases/Starbase-0.1.0-alpha.3.zip`. The previous alpha.2 package
remains intact. Unreal Editor is not required.
The archive is 1,204,334,408 bytes (approximately 1.20 GB); all 49 packaged
files match the build manifest, and ZIP CRC verification passed.

## Delivered changes

- Registered aerial imagery removes the overlapping tile footprints. Central
  terrain has finer geometry and coastal dune detail; water shading follows a
  geographic shoreline mask without an additional rectangular ocean plane.
- A continuous service road connects maintenance bays, workshops and parking.
  Service vehicles circulate before countdown; tank vents, fixtures, lighting
  and industrial details make the existing pad more active.
- Five photographic presets, adjustable optics and exposure/color/environment
  controls, and three persistent custom looks. Paused previews remain responsive
  without advancing the physical vehicle or retaining stale motion blur.
- The sun follows true east/north/up and Starbase's latitude, longitude, calendar
  and selected civil-time offset. The saved editor level uses the same function.

Implementation, data provenance, control ranges and limitations are in
[Site geography and photographic controls](../SITE_PHOTOGRAPHY.md).

## Validation of the packaged executable

Tests used isolated preferences on Windows, Ryzen 5 9600X and RTX 4070 SUPER.
The Development build retains the physical force inspector. Archive and package
hashes identify the exact tested artifacts in [the release records](0.1.0-alpha.3).

| Check | Result |
|---|---|
| Packaged input and menu audit | 69 checks passed |
| Photographic controls and rendered solar direction | 15 checks passed |
| Saved look after process restart | 2 checks passed |
| Rendered exposure, EV -1 to +1 | Mean linear luminance increased by 2.80x |
| Full Crosswind mission, fixed 15 Hz game step | Passed, 7,256 rendered frames |
| Peak altitude | 97.257 km |
| Front approach and physical capture | Passed; both rail supports, engines off |
| Eight seconds of unpowered support | 0.0468 m drift |
| Chase camera after contact | 121 frames; no discontinuity detected |
| Actual reconstruction | NVIDIA DLSS Quality, 66.7% internal resolution |
| Fresh packaged logs | No detected fatal errors, ensures or missing content |

Solar unit tests additionally verify morning/evening directions, the southern
solar culmination, seasonal changes, normalized vectors and equivalent CDT/CST
instants. Geography checks verify all 16 source extents, 24 adjoining boundaries,
the coordinate contract, pad-height preservation and a closed 1,019.72 m route.
These reports are also retained in `Docs/Validation/SitePhotography`.

Final-package imagery, the golden-hour look, the environment menu, approach and
secured captures were visually inspected. Local evidence directories are recorded
in `validation.json`. The `executable_sha256` field inherited from the test harness
identifies the bootstrap; `game_executable_sha256` identifies the actual game binary.

## Limits of this release

The site is an authored simulator layout over historical imagery. Distant land
mosaic tone differences and repetitive water detail remain visible. This batch
does not change the landing burn or flight guidance, complete the full realism
programme, or establish an FPS improvement. New scenery uses shared instances,
bounded lights and distance culling; comparative performance measurement remains
separate work. Civil-time DST selection is explicit and the moon is still artistic.

## Reproduce

```powershell
./Tools/Runtime/build_simulator.ps1
./Tools/Runtime/package_alpha.ps1 -Version 0.1.0-alpha.3
./Tools/Tests/test_site_photography.ps1 -GameExecutable <packaged-game-executable>
./Tools/Tests/test_packaged_alpha.ps1 -Version 0.1.0-alpha.3
python Tools/Runtime/archive_alpha.py Releases/Starbase-0.1.0-alpha.3 --validation Docs/Releases/0.1.0-alpha.3/validation.json
```

The package script requires committed sources and a new output directory.
Generated executables and archives remain outside Git. No remote push was made.

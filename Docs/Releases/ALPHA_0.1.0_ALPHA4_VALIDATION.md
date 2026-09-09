# Alpha.4: late ignition and physical front capture

Released build: **0.1.0-alpha.4**, 9 September 2026.
Game source commit: `11565bb0db1473510caff2c2fa5cc34dd1f1a944`.
Release tag: `v0.1.0-alpha.4`. The tag additionally records validation documents;
game source and content were not changed after packaging.

Launch `Releases/Starbase-0.1.0-alpha.4/Windows/SuperHeavySim.exe`, or extract the
complete `Releases/Starbase-0.1.0-alpha.4.zip`. The previous alpha.3 remains intact.
The Unreal Editor is not required. Keep the entire packaged Windows folder.
The portable archive is 1,204,344,002 bytes (about 1.20 GB). Its 50 files match
the package manifest and ZIP CRC verification passed.

## Delivered behaviour

Landing ignition follows an updated stopping-distance prediction using live
velocity, mass, fuel, available engines, atmosphere and valve response. The
return continues coasting lower before a stronger braking pulse and a short
final alignment. A safe fitting contact initiates thrust shutdown and gravity
transfers weight onto the two physical rails. The vehicle is never snapped to a
position, prescribed velocity or attachment at capture.

The front corridor remains enforced. Longitudinal fitting offsets within the
usable rail span no longer cause unnecessary powered recentering. Crosswind
prediction and damped lateral tracking share the force model's wind profile;
arm clearance includes the short settling motion. Delayed collision diagnostics
use consistent component coordinates, and the flight gates reject structural
contact even when the vehicle eventually obtains support.

Implementation, estimates, before/after measurements and the qualitative Flight 5
reference are documented in [Dynamic return](../DYNAMIC_RETURN.md). Site geography,
solar orientation and photographic controls from alpha.3 remain available.

## Exact packaged executable validation

Windows, Ryzen 5 9600X, RTX 4070 SUPER; isolated user preferences. The packaged
Development game ran a complete rendered Crosswind mission at 15 Hz fixed game
cadence with 120 Hz physics and NVIDIA reconstruction at 66.7% internal resolution.

| Check | Result |
|---|---:|
| Packaged input/menu checks | 69 passed |
| Rendered mission | Passed, 6,649 frames |
| Landing ignition altitude | 2,093.71 m |
| Downward speed at ignition | 398.27 m/s |
| Landing thrust duration | 30.375 s |
| Incoming body speed at first support | 0.998 m/s |
| Slow interval before contact | 4.533 s |
| Structural contacts | 0 |
| Front ingress / both rail supports / engines off | Passed |
| Drift during eight seconds of passive support | 0.139 m |
| Chase contact continuity | Passed over 121 frames |
| Missing content, fatal errors or ensures in checked packaged logs | None detected |

Home, front approach and secured screenshots from the final executable were
visually inspected. The final game binary, bootstrap, package file manifest,
archive hashes and evidence directory are recorded in [the release records](0.1.0-alpha.4).
The legacy `executable_sha256` field identifies the bootstrap;
`game_executable_sha256` identifies the actual game binary.

The source validation additionally includes 23 passing model/presentation tests,
four unpowered contact fixtures and eight full flight cases: Nominal, Crosswind
and mass-offset returns at 15/60 Hz game cadence, a stalled/variable-frame
Crosswind run, and a 240 Hz physics Crosswind spot check. All eight satisfy the
short-burn, incoming-speed, slow-interval and zero-structural-contact gates.
Raw reports and compressed comparison traces are in
[Dynamic return evidence](../Validation/DynamicReturn/README.md).

## Limits

This is an estimated vehicle and atmosphere model inspired by the reference
flight, not a reconstruction of private telemetry or a certified minimum-altitude
controller. The tower still uses its existing kinematic arm mechanism; structural
compliance and overload/failure modelling remain unfinished. A successful 240 Hz
spot check does not establish whole-return numerical convergence. Graphics and
FPS were not reworked in this batch.

## Reproduce

```powershell
./Tools/Runtime/build_simulator.ps1
./Tools/Runtime/package_alpha.ps1 -Version 0.1.0-alpha.4
./Tools/Tests/test_packaged_alpha.ps1 -Version 0.1.0-alpha.4
python Tools/Runtime/archive_alpha.py Releases/Starbase-0.1.0-alpha.4 --validation Docs/Releases/0.1.0-alpha.4/validation.json
```

Packaging requires committed source and a new output directory. Executables and
archives stay outside Git. No remote push was made.

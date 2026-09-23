# Tests

Run commands from the repository root. Keep test results in ignored
`Saved/Recovery`, or publish them as CI/release artifacts.

## Layout and naming

| Directory | Responsibility |
|---|---|
| `Python/analysis/` | Frame times, image comparisons and timestep convergence |
| `Python/releases/` | S3 publication, checksums and retry behavior |
| `Unreal/Automation/` | Native C++ model-test runner |
| `Unreal/Flight/` | Return, tower contact, failures, ground sequence and water contact |
| `Unreal/Presentation/` | Viewer, photography and startup acceptance |
| `Unreal/Packaging/` | Standalone Windows package acceptance |
| `Unreal/Assets/` | Asset contracts, geography and dependency inventory |
| `Unreal/Performance/` | Frame-time measurements and cloud captures |
| `Unreal/Inspection/` | Read-only geometry, assembly, lighting and scenery inspection |
| `Shared/` | Common assertions and evidence capture |

Names describe behavior, not an old development batch: `test_` for acceptance,
`audit_` for asset checks, `inspect_` for diagnostics, `measure_` for timing and
`capture_` for visual evidence. Python test subdirectories contain `__init__.py`
so standard unittest discovery also works on the CI Python version.

## Unreal automation

The C++ `Recovery.*` model tests live in
`Source/SuperHeavySim/Private/Recovery/Tests`, registered with Unreal's Automation
Test Framework under `WITH_DEV_AUTOMATION_TESTS`. They are discoverable from the
editor and command line; they are not Python reimplementations of the physics.

```powershell
./Tools/Runtime/build_simulator.ps1
./Tests/Unreal/Automation/run_model_tests.ps1
```

The runner invokes `Automation RunTests Recovery.`, exports the native report,
and rejects missing, skipped or failing tests even if Unreal exits with code 0.
Scripts accept `-EngineRoot` for an installation other than `D:/Engines/UE_5.8`.

## Integration and packaged acceptance

`Unreal/Presentation/test_viewer.ps1` combines asset, interface and rendered-flight audits.
Use `Unreal/Flight/test_return_scenarios.ps1` for the flight matrix. Contact, resilience,
emergency, ground sequence, photography, startup and marine scripts exercise
separate scenarios. `Unreal/Packaging/test_windows_package.ps1` checks the actual packaged binary.
They retain their current report contracts and are not Gauntlet tests.

For a focused UI check:

```powershell
./Tests/Unreal/Presentation/test_viewer.ps1 -SkipMatrix -SkipAssets -SkipFlight -MenuAudits RecoveryUIAudit
```

Use `RecoveryEarthAudit` for cameras or `RecoveryOverhaulAudit` for presentation.
Asset audits under `Unreal/Assets/` and diagnostics under `Unreal/Inspection/` run in the editor's Python
environment, not system Python. Runtime audit components remain in the game
module because Development-package acceptance uses them.

`Shared/validation_evidence.ps1` owns common acceptance assertions and provenance.
`measure_frame_times.ps1`, `measure_clouds.ps1` and `capture_cloud_views.ps1` collect
captures; tools under `Tools/Analysis/` analyze them separately.

## Offline regression tests

```powershell
python -m pip install -r Tests/Python/requirements-ci.txt
python -m unittest discover -s Tests/Python -p 'test_*.py' -v
```

These standard-library unittest fixtures cover analysis and release publication.
They run on hosted CI without Unreal, GPUs or release credentials. They do not
replace native model tests or packaged acceptance.

Epic references: [Automation Test Framework](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine)
and [Gauntlet](https://dev.epicgames.com/documentation/unreal-engine/gauntlet-automation-framework-overview-in-unreal-engine).
Gauntlet orchestrates Unreal sessions; a future migration must retain the current
flight, audio, startup and capture assertions rather than merely launching a game.

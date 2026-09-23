# Starbase Flight Simulator

An Unreal Engine 5.8 simulator of Super Heavy launch, stage separation, boostback,
atmospheric return and physical tower capture. Includes interactive failures,
flight telemetry, orbital cameras and a configurable Starbase environment.

Current source version: **0.1.0-alpha.11**. See the
[release notes](Docs/RELEASE_NOTES.md) for controls and known limitations.

## Getting started

The supported build workflow targets Windows and requires Unreal Engine 5.8,
its C++ build toolchain, and Git LFS. After cloning:

```powershell
git lfs pull
./Tools/Runtime/build_simulator.ps1
./Tools/Runtime/launch_recovery.ps1
```

Scripts accept `-EngineRoot` when Unreal is installed somewhere other than
`D:/Engines/UE_5.8`. Open `SuperHeavySim.uproject` to work in the editor;
the default map is `/Game/Starbase/Maps/L_RecoveryLab`.

Select **Launch** to start a mission. Hold the right mouse button to orbit and
left-click vehicle parts to inspect them. **Tab** selects cameras, **F** enables
free flight, **I** displays forces, **L** opens the flight computer, and **Escape**
pauses. Settings group camera, environment, graphics and audio controls.

## Repository

| Directory | Purpose |
|---|---|
| `Source/` | C++ simulation, presentation, interface and editor modules |
| `Content/` | Authored levels, Blueprints, profiles and runtime assets |
| `Config/` | Unreal project and packaging configuration |
| `Plugins/` | Required vendor files and provenance for optional NVIDIA DLSS |
| `Tools/` | Build, test and asset-authoring entry points |
| `Docs/` | Architecture, sources, release notes and measured validation |
| `.github/` | CI and release workflows |

Generated files stay in ignored `Saved/`, `Intermediate/`, `Binaries/` and
`Releases/` directories. External authoring inputs live in `../ArtSource`;
they are not required to package committed assets. Runtime physics and visual
presentation are separated; asset references and shared conventions are
centralized. See [project structure](Docs/PROJECT_STRUCTURE.md).

## Validation and releases

```powershell
python -m pip install -r Tools/Tests/requirements-ci.txt
python -m unittest discover -s Tools/Tests -p 'test_*.py' -v
./Tools/Tests/test_physics_models.ps1
./Tools/Tests/test_experience.ps1
./Tools/Runtime/package_alpha.ps1
```

[Tooling](Tools/README.md) describes focused checks and authoring entry points.
[CI and releases](Docs/CI_RELEASES.md) covers hosted checks, Unreal runners and
versioned S3 publication. Portable binaries are distributed separately from Git.

## Simulation and rendering

The force model resolves individual engines, gimbals, reaction jets, propellant
consumption and changing mass properties. Starship becomes an independent body
at separation. Physical fittings and compliant rails on torque-driven tower arms provide
capture contact. The flight computer exposes actuator failures and wind changes.

Flight coefficients are estimates, not SpaceX engineering data. This is an
experimental simulator, not flight-certified software. Rendering includes
spherical Earth scenery, volumetric effects, optional hardware Lumen and
TSR/DLAA/DLSS reconstruction. NVIDIA dependencies retain their vendor licenses
and [provenance](Plugins/NVIDIA/provenance.json).

- [Flight model](Docs/FLIGHT_MODEL.md)
- [Measurements and validation](Docs/VALIDATION.md)
- [Asset authoring](Docs/AUTHORING.md)
- [Resource credits](Docs/Credits/)

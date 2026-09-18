# Starbase Flight Simulator

An interactive Unreal Engine 5.8 simulator: launch, stage separation, boostback, ballistic coast, atmospheric control, landing burn and physical tower capture.

The current source version is `0.1.0-alpha.11`. See [launch instructions and known
limitations](Docs/Releases/ALPHA_0.1.0.md), [site and photography changes](Docs/SITE_PHOTOGRAPHY.md), and [coast, volume flow and mechanical capture](Docs/COAST_VOLUMES_TOWER.md). Reproduce the standalone package with
`./Tools/Runtime/package_alpha.ps1` from committed sources. Local release builds
live in `Releases/`; generated binaries remain outside Git.

Open `SuperHeavySim.uproject`, or launch the standalone viewer from PowerShell:

```powershell
./Tools/Runtime/launch_recovery.ps1
```

Use **Launch** to select a mission and camera. Escape pauses the flight. Hold the right mouse button to orbit; left-click parts to inspect them. Use **F** for free flight, **Tab** for cameras, and **I** for the physical force inspector. Camera sensitivity, environment, graphics and audio live in nested Settings pages. The interface is English throughout.

**I** displays the applied force vectors and inspector. **L** opens the live flight computer: fail an individual engine, jam a fin at its current angle, disable attitude jets, or adjust wind and attitude response while the simulation continues. **J / K** changes requested playback speed from 0.25× to 4×; the effective rate is limited by available frame time to preserve the physics substep budget. Experiments affect actuator forces and are logged in the flight report.

Settings → Display → Hardware ray tracing optionally enables hardware Lumen lighting and reflections, with an unsupported-device fallback. The sky, Earth and scenery changes, their data sources and remaining work are described in [World continuity and flight inspection](Docs/WORLD_CONTINUITY.md).

Build and validate:

```powershell
./Tools/Runtime/build_simulator.ps1
./Tools/Tests/test_experience.ps1
```

Both scripts accept `-EngineRoot` if Unreal is installed elsewhere. The default is `D:/Engines/UE_5.8`.

- [Controls and cleanup](Docs/CONTROLS_AND_CLEANUP.md)
- [Twelve visual workstreams and canonical authoring](Docs/VISUAL_RENEWAL.md)
- [Project structure](Docs/PROJECT_STRUCTURE.md)
- [Site geography, photographic looks and the Starbase civil-time sun](Docs/SITE_PHOTOGRAPHY.md)
- [LiDAR terrain, original volume flow and dynamic tower hardware](Docs/COAST_VOLUMES_TOWER.md)
- [Measured performance and validation](Docs/VALIDATION.md)
- [Viewer, rendering, audio and browser-delivery notes](Docs/EXPERIENCE_NOTES.md)
- [Flight model and physical capture](Docs/FLIGHT_MODEL.md)
- [150-item realism programme and release freeze](Docs/REALISM_PROGRAM.md)
- [Visual references and art direction](Docs/VISUAL_REFERENCES.md)
- [Historical guidance architecture](Docs/Archive/GNC_ARCHITECTURE.md)

Change flight parameters in `/Game/Starbase/Data/DA_RecoveryMission`. The reusable tower and director Blueprints are under `/Game/Starbase/Blueprints`; the original modular booster is under `/Game/Starbase/Vehicle`. Supporting art sources are in `../ArtSource`.

The flight coefficients are estimates. Capture is a rigid-body interaction between physical fittings and compliant rails on torque-driven arms; there is no pose snap or artificial hold after contact. Cold condensation uses heterogeneous volumes; near-ground deluge combines original sparse volume flow with participating fog for distant transport. This is an evolving simulation and visual model, not flight-certified engineering software.

The propulsion model resolves individual engine forces, bounded gimbal actuators, reaction-valve response, propellant use and moving tank mass properties. Starship becomes an independent physical body at separation. Ground conditioning accounts for vented propellant and supply from the launch site. Presentation consumes these states without controlling flight poses.

Settings → Display → Image reconstruction offers native TSR, TSR Quality and supported NVIDIA DLAA/DLSS modes. The official UE 5.8 DLSS plugin is project-local and included in Windows builds; DLSS is optional at runtime according to GPU support and viewer preference. Source, license and SHA-256 provenance are retained in `Plugins/NVIDIA/provenance.json` and `../ArtSource/ThirdParty/NVIDIA`. Frame generation is not installed.

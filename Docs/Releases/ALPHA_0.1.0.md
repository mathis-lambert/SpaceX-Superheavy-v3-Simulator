# Starbase Flight Simulator — Alpha 0.1.0

Version: `0.1.0-alpha.1`. Windows x64, Unreal Engine 5.8.

## Start

Open `Windows/SuperHeavySim.exe`. Keep the entire Windows folder together;
the executable requires its adjacent Engine and SuperHeavySim directories.
The Unreal Editor is not required. If Windows reports missing runtime libraries,
run `Windows/Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe` once.

Use a DirectX 12 / Shader Model 6 capable GPU with current drivers. The local
validation machine has a GeForce RTX 4070 SUPER. Other hardware has not yet
been qualified. Graphics and image reconstruction remain configurable.

## Controls

- Launch: choose a mission from the home screen.
- Mouse: orbit the current focus; F: free camera; Tab: camera selector.
- Escape: pause and settings; I: force inspector; L: live Flight Lab.
- J / K: decrease / increase playback speed.

The alpha is a standalone Development game build so the requested 3D force
inspector and local diagnostic reports remain available. It contains neither
the Unreal Editor nor the project's MCP and editing toolsets.

## Included

Launch conditioning and countdown, ascent, independent stage separation,
boostback, engine-off coast, grid-fin descent, landing burn and physical rail
capture through the tower's front opening. Booster and Starship loads use a
fixed 120 Hz physical clock. Fitting support is read from resolved Chaos contacts,
including during display stalls. Menus, camera selection, experiment controls,
optional DLSS and hardware Lumen are included.

## Known alpha limitations

- The guidance and aerodynamic coefficients are estimates. Whole-return
  numerical convergence remains incomplete; successful captures alone do not
  establish real-flight accuracy.
- Landing burns are still too long. Tower arms use a kinematic mechanism;
  articulation, compliance, overloads and failure behavior remain unfinished.
- Earth imagery, cloud/smoke detail, site assets and graphics performance still
  have improvements pending. This release freezes new feature work.
- Hardware validation is currently limited to the development machine.
- The original realism programme remains a backlog, not a completed feature list.

The package's `build-manifest.json` records the source commit and file hashes.
Release validation and archive checksums are recorded in the source repository
under `Docs/Releases`. Engine and vendor components retain their own licenses.

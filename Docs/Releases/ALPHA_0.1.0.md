# Starbase Flight Simulator — Alpha 0.1.0

Version: `0.1.0-alpha.6`. Windows x64, Unreal Engine 5.8.

## Start

Open `Windows/SuperHeavySim.exe`. Keep the entire Windows folder together;
the executable requires its adjacent Engine and SuperHeavySim directories.
The Unreal Editor is not required. If Windows reports missing runtime libraries,
run `Windows/Engine/Extras/Redist/en-us/vc_redist.x64.exe` once.

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

The site includes registered coastal imagery, refined central terrain, service
roads, maintenance facilities, four service vehicles and active tank vents.
**Settings → Photography** provides five factory looks, three saved looks,
camera optics, exposure/color controls and a geographic solar calendar. The
clock is local Starbase time (CDT or CST); September solar noon is around 13:25 CDT.
See `PHOTOGRAPHY.md` for controls, sources and scope.

This version delays landing ignition according to predicted stopping distance,
current mass, velocity, available thrust and engine response. The three reference
scenarios now take approximately 30–33 seconds of landing thrust, with a soft
physical fitting contact and immediate weight transfer onto the rails. See
`FLIGHT.md` for the measured comparison and model limitations.

The startup screen waits for scene assets, shaders and texture streaming, then
fades into Starbase. The first run may take longer while the renderer prepares
resources. Propulsion combines individually driven flame envelopes, locally lit
vapor, distance-delayed layered sound and restrained camera motion. Tower release
and contact sounds follow actual mechanism events. See `PROPULSION.md` for the
audio sources and loading behavior.

The coastal world now includes surveyed USGS LiDAR relief, finer regional imagery,
shared shoreline shading and less repetitive water. Original Blender flow caches
supply turbulent lit deluge volumes. Tower arms rotate on torque-driven physical
hinges; the rails have springs, dampers, travel limits and overload failure.
The Flight Lab shows their measured loads and compression. See
`COAST-VOLUMES-TOWER.md` for data coverage, reproduction and model assumptions.

The final polish increases launch and landing vapor coverage and density within
the existing volume budgets. Weak RCS pulses are more legible, ocean shading uses
directional wave scales instead of a repeating crossed texture, and the mission
requests 0.25 m/s vertical contact with less lateral oscillation.

## Known alpha limitations

- The guidance and aerodynamic coefficients are estimates. Whole-return
  numerical convergence remains incomplete; successful captures alone do not
  establish real-flight accuracy.
- Tower hardware parameters are estimates and the carriage is an ideal fixed
  bearing. Mechanical tests do not certify real structural loads or reconstruct
  SpaceX flight telemetry.
- Earth imagery, cloud/smoke detail, site assets and graphics performance still
  have improvements pending. The new site layout is authored, not a current survey.
- Hardware validation is currently limited to the development machine.
- The original realism programme remains a backlog, not a completed feature list.

The package's `build-manifest.json` records the source commit and file hashes.
Release validation and archive checksums are recorded in the source repository
under `Docs/Releases`. Engine and vendor components retain their own licenses.

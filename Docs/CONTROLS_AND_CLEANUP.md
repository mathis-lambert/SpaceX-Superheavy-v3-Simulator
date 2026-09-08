# Controls and presentation cleanup

## Input ownership

The old director registered F1/F2/F3 and number keys as scenario selectors. The
newer controller also registered F2/F3 for inspection. The director consumed those
keys first and reset the vehicle. Those director bindings are removed. Eleven
legacy input events were also removed from the vehicle Blueprint. Its externally
called actuator events and construction graph remain available.

`RecoveryInput.h` defines the viewer actions once. The player controller registers
them and the Controls page displays the same definitions. No gameplay key selects
a scenario, restarts a flight, returns home, or aborts propulsion. Those operations
are deliberate menu actions. Restart/home have a separate confirmation page.

| Key | Action |
|---|---|
| Escape | Pause/menu |
| Tab | Camera picker |
| C | Next camera |
| F | Free camera |
| H | HUD |
| I | Physical force inspector |
| L | Live flight lab |
| J / K | Slower / faster simulation |
| Space | Start an already prepared vehicle |
| Arrow keys, WASD or ZQSD | Free-camera movement |
| E / B, or Page Up / Page Down | Ascend / descend |
| Mouse / wheel | Look / zoom or free-camera speed |

No modifier, number-row symbol, numpad or function key is required. Keyboard
layout switching still depends on the OS; letter keys should follow its active
layout. The mouse and visible menu controls provide an alternative.

The live lab handles its close key in Slate's preview route, including when a
dropdown or slider owns focus. Free-camera movement uses wall time so accelerating
the mission does not accelerate camera travel. Playback still respects the flight
integration budget; the display distinguishes requested and effective rates.

## Interface and scenery

The home page has Launch, Settings and Exit. Settings are nested without a second
sidebar duplicating them. Original thin-line vector icons scale with the viewport.
Instruction paragraphs and the persistent learning overlay have been removed;
source attribution and information necessary to understand an option remain.

Cold condensation now uses two bounded heterogeneous volume components and a
continuous animated 3D density field. Its downward flow, wind displacement and
optical density follow conditioning flow, while the flight model retains the actual
vent force and propellant accounting. This is a rendering approximation, not CFD.
Deluge and ascent vapor keep their separate bounded effects.

Three unbranded cloth flags respond to wind. Two generic utility vehicles patrol
the western service-road section while the mission is Ready. They stop for flight
operations. Animated warning lamps use emissive materials; two short-range
headlight beams are limited to the vehicles, without additional shadow maps.
These are decorative objects, not driveable or surveyed vehicles.

## Project maintenance

The input configuration now contains only the desktop viewer settings. Legacy
flight action mappings, VR axes, touch UI, motion inputs and mouse smoothing were
removed or disabled. The template-map override, Android file-server configuration,
and unused Visual Studio integration were removed/disabled. Rendering, physics and
MCP authoring capabilities remain available. The GameFeatureData scan rule remains
because UE 5.8's editor feature plugins require it.

Redundant Windows audio values were removed after comparing them with the
installed engine's defaults. The four instanced site-detail materials now persist
their required usage flags, preventing fallback rendering and repeated shader
compilation on startup.

Editor-only Blueprint maintenance lives in `Source/SuperHeavySimEditor`, separate
from the runtime simulation and presentation module.

A registry traversal includes hard references, soft references, management
references, code asset loads and user levels. Vendor demo maps are not runtime
roots. It identified 267 unused package files (approximately 1.10 GiB). Their SHA256
verified copies are in `Saved/Recovery/UnusedContent`, outside active Content.
The dependency report and recovery manifest are stored there. Source Blender
artwork and third-party provenance were retained.

## Verification

`RecoveryControlsAudit` injects actual player key events and Slate key events,
before launch and during ascent. A nondefault experiment value detects even a
reset back to the same visible pose. It checks inspector/lab toggles, J/K, focus,
mission continuity and settled pause state. This covers the input-dispatch gap in
the older tests that called controller functions directly.

Run:

```powershell
./Tools/Tests/test_experience.ps1 -SkipMatrix -SkipFlight -MenuAudits RecoveryControlsAudit,RecoveryUIAudit -Reconstruction 3
```

The broader realism programme remains open. High-value next work includes:
ground material scale and asphalt seams; better wind-driven deluge breakup;
surveyed service infrastructure; proper traffic wheels/driving collisions;
adaptive exposure and reflection consistency; a flight debrief with force/failure
history; keyboard rebinding and controller support; and measured 4K performance.

## Rendering references

- [Epic: volumetric fog](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-fog-in-unreal-engine) describes its finite voxel grid and temporal filter.
- [Epic: heterogeneous volumes](https://dev.epicgames.com/documentation/unreal-engine/heterogeneous-volumes-in-unreal-engine) documents detailed local volume rendering and its quality/performance controls.

Native component bounds, material usage and runtime setup were checked against
the locally installed UE 5.8 source. Visual inspection remains necessary: a
registered volume or successful shader compilation alone does not prove quality.

# Viewer and rendering update

## Interaction

Mouse movement orbits the booster, engine, grid fin, chase, cinematic, Starbase and whole-Earth views. Free flight also uses direct mouse look. No mouse button is required. Scroll adjusts orbit distance or free-flight speed. Sensitivity is adjustable under Settings → Camera & controls. Manual movement stops the automatic cinematic orbit without resetting its angle.

The home screen leads to mission setup. Display, environment, camera and audio settings are grouped under Settings. During flight, Escape opens pause; Flight controls provides playback speed and telemetry/explanation toggles. `I` displays phase explanations and live actuator/mass/air-load readings; `[` and `]` change playback between 0.25× and 2×. `V` or Tab opens the camera network, `F` toggles free flight, and `H` toggles telemetry. Engine indicators now follow delivered thrust. All current player-facing strings are English.

## Rendering changes

- Camera placement runs after Chaos and before Unreal caches the rendered POV. Previously it ran after camera caching, producing a one-frame mismatch with the booster at high speed.
- A bounded pool of 128 volume-domain primitives replaces the earlier 1,280 translucent mesh puff components. Two 3D noise textures erode density before voxelization. Flow-aligned billows distinguish cryogenic condensation, ground steam and ascent trail, with wind transport, buoyancy and dilution. The sun and local engine lights illuminate this participating medium. Distant Niagara sprites remain a separate layer. This is an approximate density model, not a fluid solver; fine turbulence and internal self-shadowing still need improvement.
- Small nozzle lights illuminate nearby metal. Three larger lights distribute illumination along the exhaust; only one casts shadows. This removes the former 33 overlapping, very large light volumes.
- Eight tower/mount spotlights, supported fixtures and blinking red tower beacons supplement the approach lighting. Fixture placement follows the tower transform and height. Two cryogenic vents emit before ignition. A gradual night exposure change during ignition preserves illuminated surface detail.
- Full-resolution cloud tracing is retained. The cloud material uses cached shadow maps instead of a secondary shadow ray at every cloud sample. This changes the shadowing approximation while retaining the authored cloud density and noise.
- Detailed geographic imagery remains visible through the suborbital flight instead of being replaced at 3.5–16 km altitude. The globe asset remains 16K and regional imagery 8K. Sixteen new 4,000-pixel USGS local sources now feed streamed 4,096-pixel BC7 tiles, replacing the former 2,048-pixel sources. Source resolution, residency, viewing angle and baked photographic lighting still limit optical detail.
- The optional official UE 5.8 DLSS plugin provides DLAA, DLSS Quality and Balanced on supported hardware. Native TSR and TSR Quality remain available. Fog grid size compensates for reduced scene resolution to preserve approximately the native on-screen vapor footprint. This is reconstruction, not frame generation; quality and timings are measured separately.
- Industrial detail is batched into 3,481 instances across eight components with distance culling and no collision. It adds pipelines, valves, railings, ladders, equipment cabinets, roof units and cable/fence detail. This remains a modular interpretation, not a surveyed digital twin of Starbase.
- Two coincident pad slabs and their obsolete ring/cross markings were removed. The actual mount and asphalt platform remain.

## Audio

The engine loop adapts a [NASA STS-131 launch recording](https://www.nasa.gov/historical-sounds/). It is sound design, not a recording of Super Heavy. Attribution, download URL, processing steps and SHA256 are recorded in `../ArtSource/Audio/CREDITS.json`. The ambient coastal wind loop is original synthesized audio.

Engine sound is spatialized, filtered and attenuated with distance. A history of emitted sound adds propagation delay at distant observer cameras. Sound fades as the listener leaves the atmosphere. Volume is adjustable in Settings → Audio.

## Browser delivery

The existing Unreal renderer is intended for a native Windows build. [Epic Pixel Streaming](https://dev.epicgames.com/documentation/en-us/unreal-engine/pixel-streaming-in-unreal-engine) can deliver an interactive Unreal application to a browser from a GPU host. A WebGL renderer would be a separate frontend, with reduced/alternative rendering features; it is not an export switch for this UE 5.8 project. The separation of flight, presentation and interface keeps that future work manageable. No web service is deployed by this update.

## Physical limits

Flight and aerodynamic coefficients remain estimates. The capture uses rigid primitive contacts at the fittings/rails; there is no pose snap, artificial weld or active hold after support contact. Arm positioning is still kinematic; arm flex, hardware failure and CFD remain unfinished. Starship now has an independent physical body, inherited separation state, six engine forces and its own propellant budget. Its artwork and plumes follow that body. See `FLIGHT_MODEL.md` for the remaining physical approximations.

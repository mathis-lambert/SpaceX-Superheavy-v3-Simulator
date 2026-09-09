# Propulsion presence and startup

This batch changes presentation and resource preparation. The alpha.4 flight
guidance, engine response, propellant accounting and physical rail contacts stay
authoritative. No sound, camera motion or luminous tail applies a vehicle force.

## Propulsion

- Each engine's flame uses its delivered thrust, individual material instance,
  nozzle/gimbal pose and turbulence phase. A 100 ms exponential visual tail
  preserves a short luminous decay after thrust falls. It produces no thrust.
- The large plume lights use the same envelope. Their former excessive pad
  illumination is reduced; the participating vapor still scatters local light.
- Condensation, deluge and ascent vapor respond to measured ground flows and
  delivered thrust. Existing billows continue advecting after engine cutoff.
- Acoustic source histories store actual output and positions in metres. Sound
  arrives after straight-line propagation using the atmosphere's temperature
  at mean source/listener altitude. History interpolation avoids stepped
  envelopes. A fixed-capacity ring bounds memory at 8,192 samples per source.
- Roar, low rumble and crackle are mixed by received power and range; distant
  listening loses high frequencies. Doppler pitch is bounded around the Mach
  cone and camera cuts. The observer's camera never sets vehicle state.
- Engine sound fades out of external near-vacuum views. The mounted camera can
  retain a small structural rumble. Airborne camera vibration follows received
  sound; mounted vibration also follows thrust and dynamic pressure. Angular
  excitation is bounded to 0.12 degrees before the user's motion multiplier.
- Tower drive noise follows arm motion. Release noise follows mount release.
  Rail impacts are triggered once per physical support per mission, weighted by
  measured impulse, and propagate from their actual positions.
- Seven fixed loop voices and four reusable transient voices bound audio work.
  Master volume also controls transients. Restart clears propagation history.

This is an audiovisual model, not calibrated sound pressure, CFD or an acoustic
shock/refraction solver. It intentionally does not invent a sonic boom from a
simple supersonic pitch calculation.

## Sources and authored assets

The existing launch bed is adapted from [NASA's Discovery STS-131 launch sound](https://www.nasa.gov/historical-sounds/),
under the [NASA media usage guidelines](https://www.nasa.gov/nasa-brand-center/images-and-media/).
It is not a recording of Raptor engines. The source, adaptation and SHA-256 are
recorded in `ArtSource/Audio/CREDITS.json` beside the original audio.

`Tools/Art/prepare_propulsion_audio.py` creates seven original 48 kHz mono layers:
EngineRumble, EngineCrackle, CryogenicHiss, Deluge, TowerDrive, TowerContact and
MountRelease. Filtered noise, amplitude modulation and damped inharmonic modes
provide the layers; no third-party voice or music is added. The generator writes
duration, RMS, peak and hashes in `ArtSource/Audio/PROPULSION_CREDITS.json`.
`Tools/Editor/import_propulsion_audio.py` imports only these assets into
`/Game/Starbase/Audio`, with inline loading and bounded runtime voices.

## Startup

`RecoveryLoading` is a small module loaded at Unreal's PreLoadingScreen phase.
Its Slate screen uses no scene assets and covers engine initialization and first
map loading. It hands off to the matching viewport screen while Starbase builds.

The world subsystem loads the central runtime asset registry asynchronously and
retains the objects. Presentation construction waits for those objects. It then
precaches registered primitives, including hidden plume/volume components, and
waits for shader pipeline compilation and texture streaming requests to drain.
Several real frames verify that new work has not just been enqueued. Progress is
per-stage work completed; it is never a timer presented as loading percentage.
There is no deadline that silently declares missing resources ready.

Only a ready scene fades in. Viewer controls and automatic test launches wait for
this handoff. Null-RHI and commandlet simulations bypass presentation loading.
`Saved/Recovery/startup-report.json` records actual loaded/requested counts,
remaining/peak work, frame count and elapsed initialization times.

This follows [Epic's PSO precaching guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine).
Compilation concurrency is bounded to four workers to avoid unbounded memory
pressure on desktop CPUs. This prepares known resources; it is not a guarantee
that every future camera, quality setting or driver cache state is hitch-free.

## Validation

The validation tools cover model propagation and reset behavior, bounded effects,
asset dependencies, keyboard controls, scene readiness and complete physical
flights. The rendered flight also records the actual Unreal master submix and
rejects silence or clipped PCM samples. Its test-only command-line override keeps
audio active when the unattended window is unfocused; ordinary background audio
preferences are unchanged. This signal check does not replace a listening review.

Evidence is kept under `Docs/Validation/PropulsionStartup`. Release-specific
executable, startup and archive checks are recorded under `Docs/Releases`.
Frame-time observations include screenshot readback stalls and are not a
like-for-like 4K gameplay benchmark or a guarantee of stutter-free playback.

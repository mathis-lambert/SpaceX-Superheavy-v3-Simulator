# Mission controls and marine contact

## Viewer controls

The in-flight toolbar provides **Mission** and **Restart**. Mission opens paused
playback, camera, restart, home and abort controls. Restart requires confirmation;
cancelling preserves the current mission. A confirmed restart resets the selected
scenario and playback to 1x, then starts a new countdown.

**Settings > Controls > Invert vertical look** persists across application
restarts. Its initial default reverses the previous vertical direction. Hold the
right mouse button to rotate. Orbit composition uses a 45 ms exponential input
filter and zoom uses a 100 ms response in wall time. Physical vehicle following
remains immediate; no interpolation is added to the rigid body's world position.

## Water contact

The booster receives distributed Archimedes and dissipative drag loads from 80
quadrature cells approximating a sealed cylindrical hull. Chaos integrates the
translation and rotation, including the moment of each water force. There are no
water-position clamps or prescribed floating poses. Water contact cuts propulsion
and ends tower recovery; the physical hull continues moving and can settle on
its side. Restart remains available after contact.

`Tools/Data/build_physics_water.py` packs local, regional and global CPU masks from
the render masks into `Content/Starbase/Data/Water/Surface.bin`. It is staged as UFS
data and loaded once during configuration on the game thread. The immutable map
is shared with the solver; substeps perform no I/O, UObject access or allocation
of image data. Local classification resolves roughly 12 m, with progressively
coarser classification away from the launch site.

This model treats the booster as an intact, sealed hull. It does not model impact
breakup, flooding, water entry spray, seawater ingress into engines, or individual
wave motion in the force calculation. Water support is at mean spherical sea
level. The independent upper-stage model is unchanged.

`Tools/Tests/test_marine_contact.ps1` checks vertical, horizontal and 100 m/s entry
fixtures at two game cadences. Each runs for 120 simulated seconds and verifies
settling and displaced-water weight, with propulsion disabled.

## Rendering

- Water combines two warped geometric swells with a deterministic, offline
  Fourier slope field (`Tools/Data/build_water_spectrum.py`). Two differently
  scaled, rotated and moving texture samples replace eight short sine waves.
  Averaged mip levels filter unresolved slopes; the original 1024-square field
  contains no external art. Swell gradients include the warp derivatives.
- Above 12 km, clouds trace at half view resolution instead of quarter resolution.
  Returning below 10.5 km restores the near-layer mode. Ray sample reduction now
  bottoms out at 1x, and the shape volume uses uncompressed linear samples to avoid
  block-compression artifacts. The near-cloud density model is preserved.
- Booster flame mouths retain a fixed radius in atmosphere and vacuum. Pressure
  expansion starts downstream; the shared mixing plume is narrower and more
  transparent. An instanced emissive core sits inside each burning nozzle and
  tracks delivered power. Local rim lights remain separately bounded.

These cloud settings intentionally spend more GPU time at altitude. Evaluate
the matched orbital capture as well as the near-cloud view before claiming a
performance gain. A static screenshot does not establish temporal smoothness.

## References

- Epic, [Water Buoyancy Component](https://dev.epicgames.com/documentation/unreal-engine/water-buoyancy-component-in-unreal-engine): distributed volume approximation.
- NVIDIA, [Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models): wave spectra and filtered surface detail.
- Epic, [Volumetric Clouds](https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine): trace and reconstruction tradeoffs. Mode definitions were also verified against the installed UE 5.8 renderer source.

## Validation

34 automation assertions/tests passed, as did the nominal and crosswind recovery
flights, restart/cancel controls, and preference persistence after relaunch. Six
120-second water entries passed at 30/60 Hz. Evidence: `Validation/MissionMarine`.

Final stationary probes at 2560x1440 with DLSS Quality on RTX 4070 SUPER:
94 km: 74.82 FPS (cloud trace 3.10 ms); 14 km: 66.95 FPS (4.91 ms);
inside the layer: 89.07 FPS (1.86 ms). Each measured 360 frames after a 15-second
warmup. High-altitude resolution is intentionally more expensive than mode 0.

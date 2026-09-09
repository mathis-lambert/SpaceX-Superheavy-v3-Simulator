# Coast, volumetric flow and mechanical capture

Alpha.6 implementation. Scope: improve the local-to-orbital landscape,
replace simple billow structure with authored turbulent volumes, and make the
catch arms and rail supports dynamic mechanical bodies.

## Validation contract

- Geographic sources retain extents, datum, provenance and hashes. Review
  projected detail in fixed views at ground/5/20/78 km.
- Terrain borders match; the pad datum is preserved; coastline shading and
  shallow water are shared across geometry scales, without rectangular overlays.
- Offline flow simulation is reproducible and its imported sparse volumes
  render with shadows and local lighting within a bounded effect pool.
- Compare smoke quality and cost with alpha.5 using matching views.
- Arms rotate about physical hinges under bounded motor torque. Rail springs,
  dampers, travel stops and overload failure are physical and inspectable.
- No active-flight arm or vehicle pose/velocity writer substitutes for the
  actuator or contact solver. Explicit mission resets initialize hardware only.
- Unpowered support, side impact, wrong heading, overload and complete front
  returns pass their relevant success/failure expectations.
- Startup, controls, resource dependencies and the packaged executable must pass
  against the exact release manifest. Release evidence is recorded separately
  under `Docs/Releases/0.1.0-alpha.6`.

Source validation passed 25 model tests, eight mechanical fixtures and six full
returns (Nominal, Crosswind and Offset, at 15/60 Hz game cadence). Each successful
return leaves both rails supporting the unpowered booster, with intact hardware
and zero structural contacts. Terrain edge verification covers all 24 shared
borders. Fixed-camera volume isolation checks compare the rendered density with
the volume pass disabled; an active component count alone is not visual proof.

## Mechanical estimates

`FRecoveryTowerMechanics` exposes SI parameters in the tower Blueprint. Initial
engineering estimates: 65 t per arm, 2.5 t per rail, 12 MN m motor torque, 50 MN/m
rail stiffness, 4.5 MN s/m damping, 0.35 m travel and 9 MN rail break threshold.
These are simulator assumptions, not published manufacturer specifications.

The fixed carriage is an ideal grounded bearing. The two arm frames and two rails
are independent Chaos bodies; all loads flow through their joints. Joint
projection is disabled. Spring drives represent suspension hardware and have
finite force limits. Failure must leave the disconnected bodies dynamic.

Geometry places the arm pivots at the carriage front, clear of the structural
columns. The closed rail centre is four metres behind the capture axis. Guidance
requests a gap; the mechanism converts that request to a hinge target, with actual
motion determined by the torque drive and load. No catch restraint attaches the
vehicle to the tower.

The motor uses a force-mode PD drive (600 MN m/rad stiffness, 180 MN m s/rad
damping), capped at 12 MN m. It wakes sleeping bodies on new commands. Rail
load and overload events are sampled after every 120 Hz solver step, not from
game-frame notifications. The Flight Lab displays live rail loads, suspension
compression and hardware state. Guidance reserves 24 cm for transverse weight
transfer while retaining fitting overlap. The hull and arm frames still collide.

## Geography

The 12 km local area combines USGS TX_LowerRioGrande_D22 one-metre DEM tiles
(NAD83 / UTM 14N, NAVD88) with the existing coarse 3DEP fallback. Available LiDAR
covers 50.58% of that square; offshore and missing areas retain the fallback.
The raster is sampled onto a 6144-square grid (1.95 m spacing). Geometry uses
3.91 m central spacing and 11.72 m outer spacing, with explicit nested-grid edge
stitching. The measured maximum FBX boundary discrepancy is 0.40 mm. Pad height
and the spherical Earth reference remain unchanged.

The new 8192-square regional EOxCloudless mosaic covers 0.6 degrees, between
local NAIP imagery and the existing wider mosaics. Output sampling is about
8 m; this does not imply new detail beyond the Sentinel source resolution.
All nineteen surface materials share geographic sampling and coverage fades.
Land detail adds two decorrelated texture scales and close sand ripple normals.

A shared 4096-square hydrology mask supplies water coverage, signed shore
distance and an estimated optical shallows term. It is derived from elevation,
not surveyed bathymetry or a tidal model. Shore foam is bounded near the shoreline
and fades with viewing distance. Two warped wave-normal scales and up to 36 cm
of near-water vertex displacement avoid a full ocean-fluid simulation.

## Original volume flow

`bake_turbulent_volumes.py` uses Blender 5.2.1 Mantaflow to bake 64 frames of warm
inflow rolling around a low deflector in a 20 × 16 × 12 m domain. The OpenVDB noise
cache becomes one animated sparse volume texture (126 × 100 × 74 voxels; 53 MB
uncooked asset). Eight runtime instances share that streamed cache, with a six
second lifetime, advection, expansion, smooth extinction and distance fading.
Volume albedo receives local lighting and shadows; its emission is zero.

This is an offline visual flow simulation. It does not solve live fluid collisions
with every site object or model rocket-exhaust chemistry. The existing inexpensive
fog/trail pool supplies distant transport, with reduced hot-deluge density. The
two cryogenic vent fields remain a separate effect. Sparse-volume performance
must be assessed in the packaged renderer, not inferred from the pool limit.

## Reproduction and ownership

Generated source caches live under `../ArtSource`; runtime assets are under
`Content/Starbase`. Source URLs, extents, hashes and attribution are versioned in
`Docs/Research/CoastVolumes`. No new external audio or stock VFX are used here.

1. Use Python 3.11 with `Tools/Data/requirements-geospatial.txt` in an isolated
   environment. Run `prepare_coastal_lidar.py`, `prepare_coastal_shading.py`,
   `fetch_world_continuity.py --regional` and `prepare_earth_scenery.py`.
2. In Blender background mode run `build_lidar_terrain.py`,
   `bake_turbulent_volumes.py`, and `build_tower_meshes.py`.
3. In Unreal Python commandlets run `import_coast_volumes.py`,
   `build_world_continuity.py -CoastalShadingReimport` (the flag belongs on the
   commandlet command line), then `finalize_coast_mechanics.py`.
4. Run `audit_lidar_meshes.py` in Blender. Run the model, contact-fixture,
   physical-flight and experience audits, then test the packaged game.

The LiDAR builder owns all sixteen terrain meshes. The older site-landscape
pipeline leaves those meshes alone when the surveyed source is present, while
continuing to build service roads. Runtime tower geometry, authored trusses and
volume presentation are separate from the flight-force model.

## Limits

Rail and motor parameters are engineering estimates; the rigid carriage is an
ideal fixed bearing. The tests demonstrate this model's behavior, not real tower
certification. Terrain and imagery acquisition dates differ from the authored
site layout. Shallow-water color is an optical approximation. Geographic source
resolution, SVT streaming and whole-flight numerical convergence remain practical
limits rather than claims of full real-world reconstruction.

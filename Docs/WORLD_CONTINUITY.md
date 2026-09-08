# World continuity and flight inspection

Implemented 8 September 2026. The source, configuration, tools, documentation and original affected Earth/material/map/data assets were backed up in `Saved/Recovery/BeforeWorldContinuity` with 250 checked SHA-256 entries. Existing source artwork and textures were retained; the new inputs use separate folders.

## Viewer and flight controls

Chase orbit previously normalized contact velocities close to zero. Their direction changed abruptly as the booster settled on the rails, moving the camera around a large radius. Presentation now retains its last meaningful direction below 2 m/s, reacquires above 5 m/s, and bounds direction changes to 65 degrees/second. It does not change vehicle position, attitude, velocity, or contact forces.

I shows the actual engine, aerodynamic, grid-fin, gas-jet, vent and gravity forces. Their application points are drawn on the current rendered body; arrow lengths share an explicitly labelled logarithmic scale. The panel reports thrust, dynamic pressure, Mach and active experiments. The centre-of-mass marker follows the current physical mass distribution.

L opens a live, non-pausing laboratory. Individual engine failure closes the engine's valve through its existing transient model. A fin jam holds the measured surface angle; the vehicle remains free to rotate. Reaction-jet failure disables valve commands. Wind and attitude-response adjustments affect the same simulation used in normal flight. Restoring experiment inputs restores actuator availability; it does not rewind or repair a failed trajectory. Restart resets the experiments. Commands appear in flight-report events. J/K change playback speed. See the current [control registry and cleanup](CONTROLS_AND_CLEANUP.md).

Requested playback ranges from 0.25× to 4×. Effective acceleration is reduced when necessary to keep each simulation update within the existing 1/15-second interval and 120 Hz Chaos substep budget. This is forward time scaling, not replay or phase skipping. Large rendering stalls remain a limitation of the current frame-driven control loop.

Menus now use a single nested hierarchy with vector icons, separate mission, display, atmosphere, camera, sound and source pages. Display changes must be kept or reverted before leaving their confirmation page. The optional hardware setting controls Lumen global illumination and reflections, with device support checked at runtime. It is not path tracing, frame generation or a promise of faster rendering.

## Geographic rendering

All 19 Earth surface materials share geodetic coordinates, continuous global/regional/coastal imagery and a common water shading model. The sixteen local NAIP tiles blend toward that common imagery with distance and edge feathering. This removes geometry-footprint boundaries from the high-altitude imagery; it does not remove every seasonal or color difference in the original photographs.

- Continuous Gulf and coast mosaics are 8,192 pixels square, assembled without upscaling from 2,048-pixel WMS responses. Coast coverage is 2.4 degrees across; Gulf coverage is 24 degrees. Data extent and actual ground resolution must not be confused with texture dimensions.
- Local terrain uses USGS 3DEP source GeoTIFFs, resampled to a 2,048² height field over 12 km. Sixteen curved meshes have 256² cells each, approximately 11.72 m spacing. The geometry preserves the approximate 4.5 m pad datum offset and blends into the engineered apron.
- The globe has 1,024 × 512 segments. Local terrain uses Nanite; the global/regional shells retain their existing rendering policy. Their scenery meshes do not define flight collisions or a driveable landscape.
- Ground shading retains terrain normals and close surface detail. Ocean shading uses moving wave normals and distance-dependent roughness. Water classification is a color-derived approximation; tidal hydraulics, breaking waves and a surveyed shoreline mask are still absent.

Cloud entry tracing now reaches beyond the visible horizon from the 73–98 km flight corridor, with distance measured from entry into the cloud layer. Internal cloud tracing remains bounded. The procedural star field is combined with Unreal's atmospheric luminance and sun/moon disks, avoiding an opaque black replacement sky. Star appearance responds to exposure, but its distribution and brightness are artistic; this is not a calibrated astronomical camera.

## Vapor and site artwork

The subsequent [controls and cleanup wave](CONTROLS_AND_CLEANUP.md) replaces the cold-condensation rendering described below with two detailed heterogeneous volumes, and adds bounded service traffic, amber beacons and wind flags. The 128-volume pool remains for ground/flight vapor.

The local vapor material samples a project-authored periodic 128³ density field at multiple scales. Seeded erosion, variable envelopes, flow orientation and dilution replace the previous nearly uniform ellipsoids. The bounded 128-volume pool still participates in volumetric lighting. Cold condensation is emitted downward near the hull, with visual wall avoidance and negative buoyancy; wind subsequently carries it away. The deluge has overlapping outward-moving billows and the ascent trail persists behind the vehicle.

This is an optical and advection approximation, not a fluid solver. The finite fog grid still limits small turbulent structures, self-shadowing and close camera quality. Passing the volume-render audit does not certify AAA smoke quality.

Two new unbranded Blender models provide four service pickups and three towable generators, with separate paint, rubber, glass, metal and lamp materials. They are decorative, parked equipment outside the launch hardware. They do not establish a surveyed current Starbase layout or a driving mode. Their source file is `ArtSource/Starbase/Service/SiteServiceProps.blend` in the workspace above the Unreal project.

## Provenance and references

- **Sentinel-2 cloudless — https://s2maps.eu by EOX IT Services GmbH**, 2016 layer, [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Layer title, exact requests and hashes are recorded in `ArtSource/Earth/Continuity/sources.json`. [EOX licensing](https://cloudless.eox.at/pricing) distinguishes this layer from more recent restricted imagery. Historic imagery is not evidence of current launch-pad construction.
- [USGS 3DEP elevation documentation](https://www.usgs.gov/faqs/what-projection-horizontal-datum-vertical-datum-and-resolution-a-usgs-digital-elevation-model). Source `n26w098` and `n27w098` GeoTIFF hashes, georeferencing and validity coverage are in `ArtSource/Earth/Continuity/Elevation/sources.json`. The source has metre elevations referenced to NAVD88; the simulator's local pad offset is approximate.
- [Epic Sky Atmosphere reference](https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-properties-in-unreal-engine): sky-material luminance composition. [Volumetric cloud component](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UVolumetricCloudComponent): cloud entry and trace ranges.
- [NASA on photographing stars](https://science.nasa.gov/blogs/earth-matters/2011/09/28/where-are-the-stars/): a daylight Earth exposure may leave stars invisible. Artistic visibility should not be presented as physical camera calibration.
- [FAA Starbase project documents](https://www.faa.gov/space/stakeholder_engagement/spacex_starship), checked 8 September 2026. The [April 2025 final assessment](https://www.faa.gov/media/94346), section 2.2 and figure 1, records a revised Pad B/OLM 2 location. Future expansion must follow a dated layout reference; adding arbitrary duplicate towers would not establish geographic accuracy.

## Rebuild and review

Run `fetch_world_continuity.py` and `fetch_coastal_elevation.py` in Tools/Data, then `build_earth_art.py` in Blender and `prepare_earth_scenery.py` in normal Python. In Unreal, run `import_terrain_relief.py` and `build_world_continuity.py`. Existing continuous textures are reused unless `-WorldReimportTextures` is supplied.

`build_flow_noise.py` creates the vapor field; `build_volumetric_vapor.py` imports it. `build_site_service_props.py` runs in Blender, then `import_site_service_props.py` runs in Unreal. All authoring scripts are distinct from runtime simulation.

`test_experience.ps1 -SkipMatrix -Reconstruction 3 -MenuAudits RecoveryWorldAudit -ChaseReview` checks assets, the live laboratory, rendering settings, and a complete rendered catch in Chase orbit. Unit and cadence results are recorded in [Validation](VALIDATION.md).

The 100-item programme remains open. Major remaining work includes sensor estimates and failures, calibrated aerodynamic tables, slosh/thermodynamics, dynamic tower-arm mechanics, efficient terminal guidance, surveyed multi-pad scenery, a driveable world, replay/debrief, improved fluid effects and 4K performance validation.

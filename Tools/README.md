# Project tools

Run PowerShell commands from the repository root. Build and test scripts accept
`-EngineRoot`; the default is `D:/Engines/UE_5.8`. Generated output belongs in
`Saved/` or `Releases/`, never beside source files.

## Build and release

| Entry point | Purpose |
|---|---|
| `Runtime/build_simulator.ps1` | Compile the Unreal editor target |
| `Runtime/launch_recovery.ps1` | Launch the simulation from the editor build |
| `Runtime/package_alpha.ps1` | Cook and package the Windows application |
| `Runtime/build_release.ps1` | Build, validate, package and archive a release |
| `Runtime/archive_alpha.py` | Archive a validated package with checksums |
| `Runtime/publish_release.py` | Publish verified artifacts to S3 |
| `Runtime/install_dlss.py` | Install the official vendor plugin from its distribution |

See [CI and releases](../Docs/CI_RELEASES.md) for runner requirements and publication.
Packaging uses committed assets; it does not regenerate the world or require
the external authoring sources.

## Tests and analysis

Test runners and fixtures live in [Tests](../Tests/README.md).
`Analysis/` contains offline CSV, audio and image comparison tools; these do not
launch the simulator or generate assets.

## Why the authoring tools are retained

These are source generators and targeted importers, not application startup
requirements. A standalone script does not need another script to call it, but
it must have a concrete output and a current consumer. Committed Unreal assets
remain authoritative; this is not a claim that a blank checkout can recreate
all art without the external `../ArtSource` inputs and installed authoring tools.

| Source operation | Current consumer / result |
|---|---|
| `Art/build_overhaul_art.py`, `build_flight_art.py` | `Editor/import_vehicle_art.py`: Starship, RCS housings, exhaust envelope and launch mount |
| `Art/build_tower_meshes.py` | `Editor/finalize_coast_mechanics.py`: capture-arm import; other tower source geometry remains authoring material |
| `Art/build_earth_art.py` | `Editor/import_terrain_relief.py`: spherical Earth and regional terrain meshes |
| `Data/fetch_coastal_elevation.py`, `prepare_coastal_lidar.py` | `Shared/earth_geography.py` and `Art/build_lidar_terrain.py`: source elevations and blended terrain |
| `Art/build_lidar_terrain.py` | `Editor/import_coast_volumes.py`: surveyed coastal meshes |
| `Data/fetch_earth_data.py` | NASA global imagery cache only; obsolete local imagery and elevation downloads removed |
| `Data/fetch_earth_detail.py` | `Editor/import_earth_detail.py`: Registered4000 NAIP tiles with checked hashes |
| `Data/fetch_world_continuity.py`, `prepare_coastal_shading.py` | `Editor/build_world_surfaces.py`: continuous geographic imagery and coastal shading |
| `Data/build_ocean_coverage.py`, `build_water_spectrum.py` | `Editor/build_world_surfaces.py`: geographic water masks and ocean slope texture |
| `Data/build_physics_water.py` | Runtime `Content/Starbase/Data/Water/Surface.bin`; no GPU readback in physics |
| `Data/prepare_earth_scenery.py` | Terrain/site importers populate environment-profile foliage transforms |
| `Art/build_site_landscape.py` | `Editor/build_site_landscape_assets.py`: service roads and landscape source meshes |
| `Art/build_site_service_props.py` | `Editor/import_site_service_props.py`: vehicles and service equipment |
| `Art/build_wind_flag.py` | `Editor/build_site_activity_assets.py`: cloth mesh and wind material |
| `Art/build_cloud_noise.py`, `build_flow_noise.py` | `Editor/build_layered_weather.py`, `build_volumetric_vapor.py`: sampled volume fields |
| `Art/bake_turbulent_volumes.py` | `Editor/import_coast_volumes.py`: 64-frame sparse-volume cache |
| `Art/bake_vapor_atlas.py` | Source for `T_RecoveryVaporAtlas`, sampled by `Editor/build_vapor_trail.py`; texture reimport remains an editor operation |
| `Art/prepare_scene_audio.py`, `prepare_propulsion_audio.py` | Original/credited audio sources; `Editor/import_propulsion_audio.py` imports the propulsion layers; original scene loops retain editor import metadata |

`Editor/build_visual_renewal.py` is the single complete material-pass entry point.
It invokes twelve targeted builders, including stars, interpolated steam and RCS.
`Editor/import_coast_volumes.py` calls the same steam builder after import; it no
longer writes a competing single-frame graph.

`Editor/build_site_photography.py` is retained for a different operation: local
imagery reimport, road/landscape installation, shared surfaces and sun alignment.
It is a deliberate site workflow, not an alternative full material pass. Do not
run both indiscriminately; choose the operation being changed. Individual
builders remain usable for targeted edits. `Shared/` holds reused geometry,
import and material helpers; the single-caller RCS helper was folded into its
builder. See [asset authoring](../Docs/AUTHORING.md) for execution constraints.

## Adding or changing tooling

Search existing generators, importers and shared helpers before adding a script.
Extend the owner of an asset instead of adding a second writer or patch pass.
A retained tool must document its inputs, output, execution environment and
consumer. Temporary probes, one-off migrations and agent experiments belong in
ignored `Saved/`, not in the maintained tooling tree. Retire a superseded tool
in the same change and update its callers; do not leave alternate implementations.

Do not mistake syntax checks or an existing output file for proof of a reproducible
asset build. Validate changed generators in their actual authoring environment
before committing regenerated assets. External downloads, Blender bakes and full
material regeneration were not rerun by the tooling cleanup; its validation covers
call structure, Python regression tests and the existing Unreal asset graph.

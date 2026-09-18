# Visual renewal programme

Started 2026-09-18 from validated alpha 9 (`96de4b5`). This programme replaces
superseded implementations; it does not add a second rendering mode for them.
Git retains history. A package is deleted only after runtime, level, Blueprint,
soft-reference and authoring dependencies have been checked.

## First integrated pass

All twelve workstreams are started in this pass. This is a validated foundation
for continued art direction, not a claim that the environment is finished.

| Batch | Workstream | Implemented |
| --- | --- | --- |
| A | 1. Terrain at three scales | Shared geographic water classification; feathered local imagery boundaries; corrected the globe/region illumination seam |
| A | 2. Material response | Canonical steel, mechanisms, fins, concrete, cladding, roads and heat tiles; linear masks and filtered fine patterns |
| A | 3. Light and exposure | Night exposure responds to delivered thrust and viewing distance; retained bounded site lighting |
| B | 4. Propulsion | One flame authoring graph; periodic texture flow replaces repeated procedural noise; filtered shock pattern; existing pressure/thrust inputs retained |
| B | 5. Plumes | Lit condensation trail during landing burn; warped volume edges fade before bounds; optical density revised to expose internal structure |
| B | 6. Weather composition | Altitude-dependent wind shear, offset middle-level banks and elongated upper streaks without additional volume samples |
| A | 7. Ocean | Global/regional geographic water masks, detailed local hydrology, continuous BRDF, shore wetness and filtered ripples |
| B | 8. Atmospheric depth | Spherical atmosphere ground moved below the finite globe chords; continuous lighting from regional view to full planet |
| C | 9. Camera character | Saved tracking delay, fixed coastal framing and filtered telephoto behavior; viewer-only motion |
| C | 10. Ground operations | Service traffic drives forward to staging during countdown; shared road geometry; filtered flag fabric |
| C | 11. Event persistence | Shared wetness and residue from actual deluge/thrust, gradual drying, pause preservation and mission reset |
| All | 12. Temporal image stability | Footprint filtering on road markings, heat tiles, steel, fabric, wave detail and flame shocks; redundant cryogenic volume resize avoided |

## Canonical build and cleanup

- `Tools/Editor/build_visual_renewal.py` rebuilds current visual materials in
  dependency order. Run it with Unreal closed, `-run=pythonscript`, `-nullrhi`
  and `-SCCProvider=None`. Do not run material writers alongside the viewer.
- `Tools/Shared/surface_materials.py` owns the industrial, vehicle and road
  graphs. `build_world_surfaces.py` owns geographic shading. Individual effect
  builders own flames, trails, cryogenic vapor and volumetric vapor.
- `Tools/Editor/import_vehicle_art.py` imports current vehicle supporting art
  separately from material generation. Physical colliders remain independent.
- `Tools/Data/build_ocean_coverage.py` regenerates geographic water masks from
  Natural Earth land polygons. Sources, hashes and license are recorded in
  `WATER_COVERAGE_SOURCES.json`. Cartographic shorelines are not tidal surveys;
  local surveyed hydrology takes precedence around the site.
- Sixteen superseded scripts/migrations were removed; the old world-material
  entry point was renamed. The generic coast/ocean and upper-stage proxy were
  removed from the Blender art generator. Git preserves their history.
- Seven unreachable packages (20,733,244 bytes before deletion) were removed
  after a checked recovery copy and source/map/Blueprint/soft-reference audit.
  See `VISUAL_RENEWAL_REMOVED_ASSETS.json`. The final audit resolves 169 runtime
  dependencies and reports zero unreachable packages.
- Reachability no longer treats cook-directory policy as evidence of use. The
  old inventory wrongly retained even directories excluded from cooking.

## Validation and limits

Compilation and all 32 Recovery model tests pass. A rendered photographic audit
checks live settings, pause behavior, site activity, fixed coastal framing and
tracking-setting persistence across a process restart. A complete rendered
Crosswind flight passes both rail contacts, front approach, gentle-contact and
engine-shutdown checks; the actual captured audio is neither silent nor clipped.

At 2560 x 1440 with DLSS Quality on the local RTX 4070 SUPER, the matched Home
capture measured 11.12 ms mean frame time versus 11.23 ms before this pass. This
is effectively unchanged performance, not a claimed optimization gain. Separate
stationary probes measured about 90 FPS at 94 km and 87 FPS inside a cloud layer;
cloud rendering itself averaged about 0.41 ms and 1.80 ms respectively. These are
specific views, not a guaranteed minimum frame rate for a whole mission.

The orbital ocean rectangle was reproduced, then removed in a matched comparison
by correcting the atmospheric ground radius. Keep its 500 m analytical offset:
the coarse globe's triangular chords are below the nominal spherical sea level.
Moving the analytical ground up to 6370.99 km reintroduces the illumination seam.

Remaining art-direction work includes larger coherent weather fronts, breaking
up repeated distant cloud cells, richer ground-operation choreography, smoother
junctions between acquired imagery, and more convincing large steam structures.
Vapor remains a bounded optical approximation plus a shared baked flow sequence,
not a full real-time fluid simulation. Photographic settings do not establish
radiometric calibration. The geometry, masks and sea-state model have finite
resolution, and 4K worst-case performance still needs separate qualification.

## Architecture and replacement rules

- Flight dynamics are authoritative. Presentation only reads physical state.
- Each generated material has one canonical authoring function. Retire patch
  scripts that depend on the accidental structure of a previously built graph.
- Share geographic classification between all world meshes. Mesh coverage must
  not choose a different water response or lighting model.
- Prefer texture lookups and filtered analytic signals to expensive per-pixel
  procedural noise. Bound lights, volume samples and scenery instances.
- Keep reproducible inputs, build tools, assets and validation evidence separate.
- Do not report all twelve workstreams complete because one setting changed in
  each. Mark completion only after implementation and visual/performance checks.

## Reference and evidence

Use fixed solar hour, weather, camera, output resolution and reconstruction.
Retain the alpha 9 reference, new captures and raw timings under
`Saved/Recovery/VisualRenewal*`. Compare Home, ground detail, launch, cloud
traversal, orbital coast, return/catch and night. Only one GPU renderer runs at
a time. Correctness flights use fixed simulation cadence and are not FPS tests.

Sources: Epic's [cloud documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine),
Guerrilla's [Nubis Evolved](https://www.guerrilla-games.com/read/nubis-evolved),
Epic's [water shading documentation](https://dev.epicgames.com/documentation/unreal-engine/single-layer-water-shading-model-in-unreal-engine).
The chosen implementation must be measured in this project; another game's
published timings are not transferable performance guarantees.

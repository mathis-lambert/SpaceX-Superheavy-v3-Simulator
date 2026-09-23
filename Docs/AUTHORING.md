# Asset authoring

The committed map `/Game/Starbase/Maps/L_RecoveryLab` and its referenced assets
are authoritative. Packaging does not regenerate art. Source imagery, Blender
inputs and original audio live separately in `../ArtSource`; authoring scripts
may require them, while normal builds use committed packages.

## Maintained entry points

Run `Tools/Editor/build_visual_renewal.py` through Unreal's Python commandlet
with the viewer closed (`-run=PythonScript`, `-nullrhi`, `-SCCProvider=None`).
It rebuilds visual materials in dependency order; it is not a full source-asset
reconstruction or a first-run setup requirement.

- `Tools/Shared/surface_materials.py`: industrial, vehicle and road materials.
- `Tools/Editor/build_world_surfaces.py`: geographic surface shading.
- Individual effect builders: flame, trail, cryogenic and volumetric materials.
- `Tools/Editor/import_vehicle_art.py`: vehicle supporting art imports.
- `Tools/Art/`: Blender meshes, volume caches, noise and audio preparation.
- `Tools/Data/`: imagery, terrain and geographic water preprocessing.
- `Tools/Data/build_physics_water.py`: CPU water classification used by physics.

Preserve the import/source manifests in [Credits](Credits/) and vendor licenses.
The regional imagery attribution includes EOxCloudless / modified Copernicus
Sentinel data. USGS/USDA imagery, LiDAR, NASA audio and original generated content
have separate provenance files. Requested pixel size is not native survey accuracy.

## Dependency discipline

After rebaking `Tools/Art/bake_turbulent_volumes.py` and
`Tools/Art/bake_vapor_atlas.py`, build the editor with
`Tools/Runtime/build_simulator.ps1`, then run
`Tools/Editor/import_coast_volumes.py` through Unreal's Python commandlet with
`-VolumesOnly -AllowCommandletRendering -unattended -SCCProvider=None`.
The importer verifies the VDB source checksums, reimports existing volume and
atlas assets through Unreal's native reimport handler, and rebuilds the shared
interpolated steam material. Copy the updated volume `sources.json` from
`../ArtSource/Effects/TurbulentDeluge` to `Docs/Credits/flow-sources.json`.
Validate with the asset audit and a rendered flight before committing packages.

Run `Tests/Unreal/Assets/inventory_dependencies.py` in Unreal to inspect hard, soft,
management and code references before retiring packages. Cook-directory inclusion
is not proof of use. Keep runtime file data such as `Data/Water/Surface.bin`, which
is not a UObject package. Follow removals with `audit_runtime.py` and
appropriate rendered/package tests; never bulk-delete assets by folder name.

The 2026-09-23 inventory found 171 reachable packages, no unreachable packages
and no byte-identical Content files. Every constant in `RecoveryAssets.h` has a
C++ use. This is a dependency audit, not proof that every shader branch or texture
channel contributes to the final image. Further reductions require visual review
and may involve reauthoring meshes, materials or source textures.

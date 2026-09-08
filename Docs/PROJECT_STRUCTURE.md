# Project structure

The playable project is `SuperHeavySim.uproject`. The default level is `/Game/Starbase/Maps/L_RecoveryLab`.

| Location | Purpose |
|---|---|
| `Content/Starbase/Blueprints` | Reusable launch tower and mission director |
| `Content/Starbase/Data` | Configurable flight and environment profiles |
| `Content/Starbase/Maps` | Playable spherical Earth / Starbase level |
| `Content/Starbase/Vehicle` | Original modular booster, engines, fins and their materials |
| `Content/Starbase/Meshes`, `Materials`, `Textures` | Supporting flight artwork and geographic Earth tiles |
| `Content/Starbase/FX`, `Audio` | Propulsion trail and sound loops |
| `Content/ThirdParty` | Referenced environment assets, retaining their vendor structure |
| `Source/SuperHeavySim/.../Recovery/Flight` | Mission lifecycle, guidance, force application, telemetry, tower contacts |
| `Source/SuperHeavySim/.../Recovery/Presentation` | Cameras, exhaust, participating vapor, sky, site lighting, audio |
| `Source/SuperHeavySim/.../Recovery/Interface` | Player controller, Slate menus, native HUD |
| `Source/SuperHeavySim/.../Recovery/Shared` | Asset references, geometry conventions and interface styles |
| `Source/SuperHeavySim/.../Recovery/Tests` | Opt-in integration audits, disabled during normal play |
| `Source/SuperHeavySimEditor` | Editor-only Blueprint maintenance; excluded from runtime targets |
| `Tools/Editor` | Unreal asset and scene authoring |
| `Tools/Art` | Blender artwork, vapor atlas and audio preparation |
| `Tools/Data` | Geographic data downloads and processing |
| `Tools/Shared` | Material graph helpers, project paths and geographic transforms |
| `Tools/Runtime` | Launch/build entry points |
| `Plugins/NVIDIA` | Unmodified official optional DLSS runtime plugins and provenance |
| `Tools/Tests` | Asset audits, flight tests and performance analysis |
| `Tools/Migrations` | Historical one-time transformations; not part of normal rebuilding |
| `../ArtSource` | Original 3D files, Earth imagery, generated flight artwork and credited audio |
| `Saved/Recovery` | Local reports, CSV captures and verified recovery snapshots |
| `Saved/Recovery/UnusedContent` | SHA256-verified copies of removed prototype, template and unused packages |
| `Docs/Archive` | Historical reports and documentation from earlier iterations |

Unreal packages were moved with AssetTools, preserving object references. Source/import paths were updated separately. Runtime asset strings are centralized in `RecoveryAssets.h`; class names were retained, so moving C++ headers does not rename serialized Unreal classes.

The vehicle's render meshes use Nanite and simple collision policy. Dedicated primitive hull, catch fitting and tower rail shapes define physical contact. The decorative meshes do not provide detailed triangle collision to the flight solver.

The former `DA_SuperHeavy_PhaseProfile` uses the deleted class `SuperHeavyFlightPhaseProfile`; its original bytes are preserved for historical recovery. It is not a usable modern flight profile. The active configuration is `DA_RecoveryMission`.

The complete pre-migration snapshot is in `Saved/Recovery/BeforeRestructure`, with SHA256 hashes for 537 files. `filesystem-migration.json` records filesystem moves. These local recovery files are intentionally excluded from Git.

`RecoveryPropulsion.cpp`, `RecoveryMassProperties.cpp`, `RecoveryStageDynamics.cpp` and `RecoveryGroundSystems.cpp` separate actuator forces, analytical mass properties, stage/launch connections and ground conditioning. Blueprint visual actuator commands are issued only by `RecoveryPresentationComponent`. Headless flights use the same force model. `RecoverySiteDetailsComponent` builds bounded, instanced decorative geometry without adding physical contacts.

Earth tile downloads and imports have separate entry points (`Tools/Data/fetch_earth_detail.py`, `Tools/Editor/import_earth_detail.py`). Source imagery lives under `ArtSource/Earth/Detail4000`; the geographic tile names remain stable in Unreal. Re-import backups and source hashes are preserved separately from runtime packages.

Continuous imagery and source DEMs are in `ArtSource/Earth/Continuity`; updated Blender Earth meshes have their own `Continuity/Meshes` folder. `earth_geography.py` is the shared elevation/coordinate implementation for terrain and scenery. Generic service equipment lives in `ArtSource/Starbase/Service` and imports into `Content/Starbase/Meshes/Starbase` and `Materials/Starbase`.

`RecoveryFlightInspection` owns physical force samples and experiment inputs. `RecoveryForceDisplayComponent` consumes those samples without changing physics. `RecoveryCameraTracking` holds only presentation state. Flight experiments are exposed by the interface and recorded through one event-writing helper. `RecoveryWorldAudit` is opt-in and exercises the live laboratory, time scaling and rendering settings.

`RecoveryInput.h` is the shared key/action registry for the controller and Controls page. The vehicle Blueprint and director no longer register viewer keys. `RecoveryCryogenicVapor.cpp` owns local continuous condensation volumes; `RecoverySiteActivityComponent` owns decorative traffic, beacons and wind flags. The empty template GameMode class was removed; the active flight GameMode remains.

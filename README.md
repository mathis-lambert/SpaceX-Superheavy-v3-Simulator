# SuperHeavySim

`SuperHeavySim` is an Unreal Engine project focused on modeling and commanding a **Super Heavy V3** booster.

The project provides a modular vehicle foundation with:

- one main vehicle actor
- individually encapsulated engines
- individually encapsulated grid fins
- a clear actuator command API

The goal is to expose a booster that can be driven cleanly by an external control layer through a:

```text
Guidance -> Navigation -> Control
```

architecture.

## Goals

- represent a Super Heavy V3 booster inside Unreal Engine
- expose a stable command API for engines and grid fins
- keep mesh, pivot, and local-axis details inside the Blueprints
- prepare the project for propulsion, aerodynamics, and control law integration

## Requirements

- Unreal Engine `5.7`
- `git`
- `git-lfs`

## Installation

Clone the repository, then fetch Unreal assets through Git LFS:

```bash
git clone git@github.com:mathis-lambert/SpaceX-Superheavy-v3-Simulator.git
cd SpaceX-Superheavy-v3-Simulator
git lfs install
git lfs pull
```

Then open:

- [SuperHeavySim.uproject](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/SuperHeavySim.uproject)

## Important: Git LFS

Unreal asset files (`.uasset`, `.umap`) are tracked through Git LFS.

Without `git-lfs`, Unreal will not load assets correctly. Asset files will appear as text files containing LFS pointers instead of binary data.

Quick check:

```bash
file Content/Maps/Test/L_Test_SuperHeavy.umap
```

The result must not be `ASCII text`.

## Enabled Plugins

The project currently enables:

- `ModelingToolsEditorMode`
- `SunPosition`
- `GeoReferencing`

## Repository Structure

```text
Config/
Content/
  Maps/
    Test/
  SimBlank/
  SuperHeavy/
    Blueprints/
    Data/
    Materials/
    Meshes/
    Textures/
Source/
SuperHeavySim.uproject
```

## Important Directories

- [Docs/GNC_ARCHITECTURE.md](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Docs/GNC_ARCHITECTURE.md)
  C++/Blueprint contract for the vehicle GNC stack.

- [Content/SuperHeavy/Blueprints](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/SuperHeavy/Blueprints)
  Main vehicle Blueprints.

- [Content/Maps/Test/L_Test_SuperHeavy.umap](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/Maps/Test/L_Test_SuperHeavy.umap)
  Main booster test map.

- [Content/SuperHeavy/Meshes/Imported_Clean](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/SuperHeavy/Meshes/Imported_Clean)
  Cleaned and renamed meshes prepared for Unreal integration.

- [Source/SuperHeavySim](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Source/SuperHeavySim)
  Unreal C++ gameplay, vehicle API, and GNC code.

## Architecture

### BP_SuperHeavy

[BP_SuperHeavy.uasset](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/SuperHeavy/Blueprints/BP_SuperHeavy.uasset)

`BP_SuperHeavy` is the main vehicle actor.

Responsibilities:

- own the engines
- own the grid fins
- initialize child actors
- route commands to actuators
- expose the vehicle API
- host the `SuperHeavyGncComponent`

Main functions:

- `InitializeEngines()`
- `GetEngine(EngineId)`
- `SetEngineGimbal(EngineId, Pitch, Roll)`
- `SetEngineThrottle(EngineId, Throttle)`
- `InitializeGridFins()`
- `GetGridFin(GridFinId)`
- `SetGridFinAngle(GridFinId, Angle)`

For C++ GNC integration, `BP_SuperHeavy` should be parented to `SuperHeavyVehicleActor` and implement:

- `SetEngineThrottleCommand(EngineId, Throttle)`
- `SetEngineGimbalCommand(EngineId, PitchDeg, RollDeg)`
- `SetGridFinAngleCommand(GridFinId, AngleDeg)`

The C++ base expands collective group commands to individual engine and grid-fin IDs.

### BP_RaptorEngine

[BP_RaptorEngine.uasset](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/SuperHeavy/Blueprints/BP_RaptorEngine.uasset)

`BP_RaptorEngine` represents a single engine.

Logical structure:

```text
Root
└── GimbalPivot
    ├── EngineMesh
    └── ThrustSocket
```

Main variables:

- `EngineId`
- `IsGimbaled`
- `TargetThrottle`
- `ActualThrottle`
- `TargetPitch`
- `ActualPitch`
- `TargetRoll`
- `ActualRoll`
- `MaxGimbalAngle`
- `GimbalRateDegSec`
- `ThrottleRatePerSec`

Main functions:

- `SetGimbal(Pitch, Roll)`
- `SetThrottle(Throttle)`
- `UpdateActuator(DeltaTime)`

Principles:

- `SetGimbal` writes a command target
- `SetThrottle` writes a command target
- `UpdateActuator` interpolates the actual state
- `GimbalPivot` applies engine rotation

### BP_GridFin

[BP_GridFin.uasset](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/SuperHeavy/Blueprints/BP_GridFin.uasset)

`BP_GridFin` represents a single grid fin.

Logical structure:

```text
Root
└── FinPivot
    ├── AeroReference
    └── FinMesh
```

Main variables:

- `GridFinId`
- `TargetAngle`
- `ActualAngle`
- `MaxDeflectionAngle`
- `DeflectionRateDegSec`
- `InvertSign`
- `ReferenceArea`
- `DragCoefficient`
- `LiftCoefficient`
- `AeroEnabled`

Main functions:

- `SetAngle(AngleCmd)`
- `UpdateActuator(DeltaTime)`

Principles:

- `SetAngle` clamps the requested command
- `InvertSign` absorbs local direction differences
- `UpdateActuator` interpolates the actual fin angle
- `FinPivot` applies fin rotation

## Naming Conventions

### Grid fins

- `GF_XP`
- `GF_XM`
- `GF_YM`

### Engines

- `R01..R20`: fixed outer engines
- `RGI01..RGI10`: gimbaled inner engines
- `RGC01..RGC03`: gimbaled center engines

## Current Integration State

- `BP_SuperHeavy` drives engines and grid fins through child actors
- all `33` engines are integrated through `BP_RaptorEngine`
- the `3` grid fins are integrated through `BP_GridFin`
- initialization and lookup functions are in place
- propulsion is applied through the vehicle physics body
- engine exhaust VFX are driven by engine throttle
- the C++ GNC foundation is in place
- the test map is usable to validate actuators

## Unreal Workflow

### Working map

The main test map is:

- [L_Test_SuperHeavy.umap](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Content/Maps/Test/L_Test_SuperHeavy.umap)

### Note about Child Actor Templates

When a child Blueprint changes, Unreal may keep stale data on an instance already placed in the map.

Typical symptoms:

- `EngineId` or `GridFinId` stay `None` at runtime
- initialization finds actors but not their identifiers

Recommended fix:

1. compile the affected Blueprints
2. save them
3. delete the existing `BP_SuperHeavy` instance from the map
4. place a fresh instance in the scene

## C++ GNC Base

The project includes a C++ GNC foundation:

- [SuperHeavyGncComponent.h](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Source/SuperHeavySim/Public/GNC/SuperHeavyGncComponent.h)
  Fixed-rate guidance/control component with telemetry.

- [SuperHeavyGncTypes.h](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Source/SuperHeavySim/Public/GNC/SuperHeavyGncTypes.h)
  Shared state, target, actuator command, telemetry, and phase types.

- [SuperHeavyFlightPhaseProfile.h](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Source/SuperHeavySim/Public/GNC/SuperHeavyFlightPhaseProfile.h)
  DataAsset-based flight phase configuration.

- [SuperHeavyVehicleControlInterface.h](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Source/SuperHeavySim/Public/GNC/SuperHeavyVehicleControlInterface.h)
  Stable command interface between GNC and vehicle actor.

- [SuperHeavyVehicleActor.h](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/Source/SuperHeavySim/Public/Vehicle/SuperHeavyVehicleActor.h)
  C++ vehicle base that routes grouped actuator commands.

## Assets

The repository contains:

- booster meshes
- engine meshes
- grid fin meshes
- textures and materials
- imported and cleaned Unreal assets

## Next Steps

- reparent `BP_SuperHeavy` to `SuperHeavyVehicleActor`
- implement the three atomic actuator command events in `BP_SuperHeavy`
- create and assign a `SuperHeavyFlightPhaseProfile`
- validate a first automatic phase in PIE
- add aerodynamics
- expand guidance beyond vertical speed / altitude / attitude hold
- add LQR/MPC controllers behind the same actuator command interface

## Current Limitations

- grid fin aerodynamics are not yet connected
- lateral landing guidance is not yet implemented
- the C++ GNC component still needs a full PIE validation pass through `BP_SuperHeavy`

## Git

Unreal assets are tracked through Git LFS:

- [`.gitattributes`](/Users/mathis.lambert/Documents/_PERSO/Projets.nosync/superheavy_sim/.gitattributes)

Active rule:

```text
Content/** filter=lfs diff=lfs merge=lfs -text
```

The repository ignores, among others:

- `Binaries`
- `DerivedDataCache`
- `Intermediate`
- `Saved`
- `.DS_Store`

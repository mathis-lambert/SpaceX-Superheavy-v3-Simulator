# Super Heavy Autopilot Architecture

This document defines the C++/Blueprint contract for the Super Heavy mission autopilot.

## Design Rules

- C++ owns mission state, phase sequencing, navigation, guidance, control, actuator commands, validation, and telemetry.
- Blueprint owns Unreal asset wiring: meshes, child actors, Niagara, cameras, and atomic actuator calls.
- Control logic uses SI units internally: meters, meters per second, kilograms, newtons.
- Unreal centimeters are converted at the navigation boundary.
- Attitude control uses quaternion/body-frame error.
- The autopilot never directly touches engine child actors, grid fin child actors, meshes, pivots, or Niagara systems.

## Runtime Flow

```text
SuperHeavyMissionProfile
  -> SuperHeavyAutopilotComponent::StartAutopilotMission
  -> SuperHeavyNavigationComponent::CaptureNavigationState
  -> mission phase sequencer
  -> guidance/control law
  -> FSuperHeavyActuatorCommand
  -> SuperHeavyVehicleControlInterface::ApplyActuatorCommand
  -> SuperHeavyVehicleActor group routing
  -> BP_SuperHeavy atomic Blueprint functions
  -> BP_RaptorEngine / BP_GridFin / cameras / VFX
```

## Source Layout

```text
Source/SuperHeavySim/
├── Public/
│   ├── Autopilot/
│   ├── Control/
│   ├── Navigation/
│   ├── Telemetry/
│   ├── Vehicle/
│   └── Logging/
└── Private/
    ├── Autopilot/
    ├── Control/
    ├── Navigation/
    ├── Vehicle/
    └── Logging/
```

## Core C++ Types

`USuperHeavyMissionProfile`

- DataAsset defining a complete mission.
- Contains launch transform, landing transform, altitude reference, initial phase, abort phase, and phase configs.
- Owns editable/tweakable phase parameters and transition conditions.

`USuperHeavyAutopilotComponent`

- Main runtime API for UI and game logic.
- Starts, stops, aborts, and restarts missions.
- Runs a fixed-rate control loop.
- Executes phase transitions from the active mission profile.
- Computes full XYZ guidance/control commands for landing phases.
- Emits `FSuperHeavyTelemetry`.

`USuperHeavyNavigationComponent`

- Reads Unreal physics state from the configured physics component.
- Converts Unreal units to SI.
- Produces `FSuperHeavyNavigationState`.
- Computes position and velocity relative to the mission landing target.

`Control`

- Contains PID controllers, actuator command types, actuator limits, command saturation, thrust estimates, and quaternion/body-frame math.

`Telemetry`

- Contains UI/debug snapshots.
- UI should read telemetry; UI should not recompute control state.

`ASuperHeavyVehicleActor`

- C++ base class intended as parent of `BP_SuperHeavy`.
- Implements `SuperHeavyVehicleControlInterface`.
- Expands grouped actuator commands to engine/gridfin IDs.
- Leaves only atomic actuator/camera implementation to Blueprint.

## Blueprint Contract

`BP_SuperHeavy` should inherit from `SuperHeavyVehicleActor`.

Keep these Blueprint functions:

- `InitializeEngines`
- `InitializeGridFins`
- `GetEngine`
- `GetGridFin`
- `SetEngineThrottle`
- `SetEngineGimbal`
- `SetGridFinAngle`
- `SetActiveCameraByIndex`

Implement these C++ BlueprintNativeEvent overrides:

- `SetEngineThrottleCommand(EngineId, Throttle)` -> `SetEngineThrottle`
- `SetEngineGimbalCommand(EngineId, PitchDeg, RollDeg)` -> `SetEngineGimbal`
- `SetGridFinAngleCommand(GridFinId, AngleDeg)` -> `SetGridFinAngle`
- `SetActiveCameraByIndexCommand(CameraIndex)` -> `SetActiveCameraByIndex`

Blueprint should not own:

- phase transitions
- autopilot modes
- landing-burn logic
- XYZ guidance
- PID/controller calculations
- TWR/velocity/altitude recomputation

## Required Components On `BP_SuperHeavy`

- `SuperHeavyNavigationComponent`
- `SuperHeavyAutopilotComponent`

Navigation:

- `PhysicsComponentName = COL_Body_Main`
- `AltitudeReferenceWorldZCm` can be overwritten by the mission profile.

Autopilot:

- assign `MissionProfile`
- leave `NavigationComponentName` empty unless there are multiple navigation components
- keep command mapping as validated in Unreal:
- `PitchControlBodyAxis = Body Y`
- `RollControlBodyAxis = Body X`
- `GimbalPitchCommandSign = 1`
- `GimbalRollCommandSign = 1`

## Mission Profile Setup

Create a `SuperHeavyMissionProfile` DataAsset.

Mission-level fields:

- `MissionId`
- `DisplayName`
- `Target.LaunchTransform`
- `Target.LandingTransform`
- `Target.AltitudeReferenceWorldZCm`
- `InitialPhase`
- `AbortPhase`
- `bResetVehicleToLaunchTransformOnStart`
- initial linear/angular velocity

Recommended phases:

- `GroundIdle`
- `Liftoff`
- `Ascent`
- `MainEngineCutoff`
- `Coast`
- `Boostback`
- `Entry`
- `Approach`
- `LandingBurn`
- `Touchdown`
- `Abort`

Each phase config contains:

- `GuidanceMode`
- `ControlRateHz`
- target altitude / target velocity / target attitude
- `bUseMissionLandingTarget`
- PID gains
- actuator limits
- engine group usage
- transitions

Landing phases must use the mission landing target. The landing controller consumes:

- world position XYZ
- world velocity XYZ
- landing target XYZ
- landing target velocity
- quaternion attitude error
- body angular velocity
- mass
- available thrust

## Runtime API For UI

Use only these high-level functions from UI/input:

- `StartAutopilotMission()`
- `StopAutopilot()`
- `EnterManualMode()`
- `AbortMission()`
- `RestartMission()`
- `SetManualCommand(Command)`
- `GetTelemetry()`
- `GetDebugState()`

Do not call individual phases from UI during normal operation.

## Coordinate Conventions

- World up: `+Z`.
- Booster body up: local `+Z`.
- Thrust socket direction: local `+Z`.
- Pitch-up command moves Raptors toward `-Y`.
- Pitch-down command moves Raptors toward `+Y`.
- Roll-right command moves Raptors toward `-X`.
- Roll-left command moves Raptors toward `+X`.
- Autopilot attitude mapping uses body `Y` for pitch control and body `X` for roll control.
- Gimbal pitch and roll command signs are both `+1`.
- Grid fins rotate around their local `X`; sign inversion stays inside `BP_GridFin`.

## Minimal Unreal Setup

1. Reparent `BP_SuperHeavy` to `SuperHeavyVehicleActor`.
2. Implement the four BlueprintNativeEvent overrides listed above.
3. Add `SuperHeavyNavigationComponent`.
4. Add `SuperHeavyAutopilotComponent`.
5. Create a `SuperHeavyMissionProfile`.
6. Assign the mission profile to `SuperHeavyAutopilotComponent`.
7. Configure phases and transitions in the mission profile.
8. UI calls `StartAutopilotMission`.
9. HUD reads `GetTelemetry()` on a timer.

## Extension Path

- Add aerodynamics as force/torque models fed by navigation state and actuator state.
- Replace PID control laws with LQR/MPC behind the same `FSuperHeavyActuatorCommand` output.
- Add launch pad / tower DataAssets and reference them from mission profiles.
- Add automated tests for mission validation, transition conditions, and controller outputs.

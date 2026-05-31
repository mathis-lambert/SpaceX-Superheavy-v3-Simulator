# Super Heavy GNC Architecture

This document defines the C++/Blueprint contract for the Super Heavy vehicle guidance, navigation, and control stack.

## Design Rules

- GNC code owns guidance state, control laws, actuator commands, and telemetry.
- Vehicle code owns actuator routing and physical components.
- Blueprint implements only vehicle-specific lookups and component calls.
- Control logic uses SI units internally: meters, meters per second, kilograms, newtons.
- Unreal unit conversion stays at system boundaries: world transforms are read in centimeters and converted immediately.
- Attitude control uses quaternion/body-frame error, not Euler pitch/roll deltas.
- The controller never directly touches child actor components, meshes, pivots, or Niagara systems.

## Runtime Flow

```text
SuperHeavyFlightPhaseProfile
  -> SuperHeavyGncComponent::SetFlightPhase
  -> SuperHeavyGncComponent fixed-rate control step
  -> FSuperHeavyActuatorCommand
  -> SuperHeavyVehicleControlInterface::ApplyActuatorCommand
  -> ASuperHeavyVehicleActor group routing
  -> BP_SuperHeavy actuator functions
  -> BP_RaptorEngine / BP_GridFin
```

## Core C++ Types

`USuperHeavyGncComponent`

- Captures vehicle state from the physics component.
- Runs the fixed-rate control loop, default 100 Hz.
- Applies guidance modes and attitude hold.
- Emits `FSuperHeavyActuatorCommand`.
- Exposes `FSuperHeavyGncTelemetry` for UI/debug.
- Exposes `FSuperHeavyGncDebugState` with control errors, thrust demand, raw command, and saturation flags.
- Exposes runtime APIs for assigning a phase profile, setting target bundles, and validating phase configuration.

`USuperHeavyFlightPhaseProfile`

- DataAsset that stores editable phase configs.
- Provides `FindConfigForPhase`, `IsPhaseConfigured`, and `ValidateProfile`.
- Keeps flight phase tuning out of Blueprint graphs.

`ASuperHeavyVehicleActor`

- C++ base class intended as the parent of `BP_SuperHeavy`.
- Implements `SuperHeavyVehicleControlInterface`.
- Routes command groups to engine and grid-fin IDs.
- Leaves only atomic actuator calls for Blueprint implementation.

`USuperHeavyVehicleControlInterface`

- Stable command boundary between GNC and the vehicle actor.
- Exposes `ApplyActuatorCommand(FSuperHeavyActuatorCommand)`.

## Blueprint Contract

`BP_SuperHeavy` should be reparented to `SuperHeavyVehicleActor`.

Implement these BlueprintNativeEvent overrides:

- `SetEngineThrottleCommand(EngineId, Throttle)`
- `SetEngineGimbalCommand(EngineId, PitchDeg, RollDeg)`
- `SetGridFinAngleCommand(GridFinId, AngleDeg)`

Recommended Blueprint routing:

```text
SetEngineThrottleCommand
  -> SetEngineThrottle(EngineId, Throttle)

SetEngineGimbalCommand
  -> SetEngineGimbal(EngineId, PitchDeg, RollDeg)

SetGridFinAngleCommand
  -> SetGridFinAngle(GridFinId, AngleDeg)
```

`ASuperHeavyVehicleActor` handles group expansion:

- `OuterThrottle` -> `R01..R20`
- `InnerThrottle` -> `RGI01..RGI10`
- `CenterThrottle` -> `RGC01..RGC03`
- `InnerGimbalPitchDeg/RollDeg` -> `RGI01..RGI10`
- `CenterGimbalPitchDeg/RollDeg` -> `RGC01..RGC03`
- `GridFinXP/XM/YMCommandDeg` -> `GF_XP/GF_XM/GF_YM`

## Coordinate Conventions

- World up: `+Z`.
- Booster body up: local `+Z`.
- Thrust socket direction: local `+Z`.
- Pitch-up command moves Raptors toward `-Y`.
- Pitch-down command moves Raptors toward `+Y`.
- Roll-right command moves Raptors toward `-X`.
- Roll-left command moves Raptors toward `+X`.
- GNC attitude mapping uses body `Y` for pitch control and body `X` for roll control.
- Gimbal pitch and roll command signs are both `+1`.
- Grid fins rotate around their local `X`; sign inversion stays inside `BP_GridFin`.

## Flight Phase Profile Setup

Create a `SuperHeavyFlightPhaseProfile` DataAsset and add one config per automatic phase.

For each phase:

- Set `Phase`.
- Set `bEnableGnc`.
- Set `GuidanceMode`.
- Set `bEnableAttitudeHold`.
- Set `ControlRateHz`.
- Set `Targets`.
- Set `ActuatorLimits`.
- Set engine group usage.
- Add PID overrides only when the phase needs different gains.

Call `ValidateProfile()` before using the profile. Treat `Errors` as blocking. Treat `Warnings` as setup reminders.

At runtime, prefer these component APIs:

- `SetPhaseProfile(Profile, bLogValidation)`
- `ValidatePhaseProfile(bLogResult)`
- `SetControlTargets(Targets)`
- `SetFlightPhase(Phase)`

## Minimal PIE Test

1. Reparent `BP_SuperHeavy` to `SuperHeavyVehicleActor`.
2. Implement the three atomic actuator events.
3. Add or keep `SuperHeavyGncComponent` on `BP_SuperHeavy`.
4. Assign `PhysicsComponentName = COL_Body_Main`.
5. Create and assign a `SuperHeavyFlightPhaseProfile`.
6. Validate the profile.
7. In BeginPlay or UI input, call `SetFlightPhase(LandingBurn)`.
8. Watch `GetLastTelemetry()` for phase, guidance mode, vehicle state, TWR, and last command.
9. Watch `GetLastDebugState()` for control errors, raw throttle demand, required thrust, available thrust, and saturation flags.

## Extension Path

- Add guidance modes without changing vehicle Blueprint routing.
- Add LQR/MPC controllers by producing the same `FSuperHeavyActuatorCommand`.
- Add navigation filters by improving `FSuperHeavyVehicleState` generation.
- Add UI by reading `FSuperHeavyGncTelemetry`, not by coupling UI to control internals.

# Propulsion integration and scheduling — 8 September 2026

## Scope and architecture

`RecoveryPropulsionModel` owns the booster engine valve integration and gimbal
allocation. Inputs and outputs contain values only; the model accesses no actor,
world, component, UI or material. `RecoveryPropulsion.cpp` binds these values to
Chaos forces at the measured engine mounts. Each engine stores both its endpoint
thrust (for telemetry and visual effects) and its delivered step impulse (for
fuel and mechanics). All 33 engines share the same fuel budget.

An opening valve follows the estimated first-order response
`dF/dt = (target - F) / tau`. A closing valve has a finite seating rate. The
integration now includes the complete impulse during each step, including a
valve that finishes closing partway through it. The previous code applied the
end-of-step thrust for the entire interval. Fuel exhaustion limits the sum of
per-engine impulses and leaves no endpoint thrust in an empty engine.

These equations describe the existing estimated valve model. They are not a
validated Raptor startup model. Gimbal direction is still advanced once per
control step; a time-varying gimbal direction is not analytically integrated.
RCS, upper-stage dynamics and the guidance state machine still need migration.

## Independent checks

Unreal editor builds `propulsion-core-build.log` and `physics-cadence-build.log`
succeed. `Saved/Recovery/PropulsionCoreUnit/index.json` reports eight passing
tests, including three new tests of the production engine-bank functions:

- Analytic opening and triangular shutdown impulses are invariant under unequal
  partitions at 15, 30, 60, 120 and 240 Hz, within `1e-6 N s`.
- Engine failure contributes no opening impulse; the entire bank obeys the
  available exhaust-momentum budget; an empty bank cannot emit residual thrust.
- Nozzle forces obey gimbal cone/rate limits and preserve thrust magnitude.
  The reported moment is the sum of actual force/lever-arm cross products.

These checks isolate valve integration and realizable forces. They do not prove
whole-flight convergence, contact convergence or independence from render rate.

## Measured baseline

The pre-change executable from checkpoint `f454eea` completed all nine physical
flights at 15, 30 and 60 game updates/s in Nominal, Crosswind and Offset scenarios.
Reports are retained as `Saved/Recovery/Physical_{scenario}_{rate}.{json,csv,log}`.
The nominal baseline still varies with the update cadence:

| Game updates/s | Apogee m | Main fuel consumed kg | Mission s | Landing burn s |
|---:|---:|---:|---:|---:|
| 15 | 97,268.94 | 3,566,482.73 | 395.00 | 68.53 |
| 30 | 97,237.19 | 3,567,625.17 | 397.37 | 70.90 |
| 60 | 97,225.25 | 3,568,631.57 | 398.70 | 72.22 |

The post-change nine-flight matrix passes, recorded separately under
`PropulsionCore`. All flights retain both rail contacts, engine shutdown, fuel
balance and physical passive support. The largest absolute propellant-balance
error is `3.40e-8 kg`. Source/module hashes are in `PropulsionCore-source.json`.

| Game updates/s | Apogee m | Main fuel consumed kg | Mission s | Landing burn s |
|---:|---:|---:|---:|---:|
| 15 | 97,237.18 | 3,568,118.87 | 394.87 | 68.47 |
| 30 | 97,223.35 | 3,568,641.61 | 396.50 | 70.07 |
| 60 | 97,214.39 | 3,569,412.89 | 399.52 | 73.07 |

The apogee range decreased, but terminal duration still differs. These results
do not justify calling the complete flight frame-rate independent. This matrix
varies both game control and effective solver steps; a later convergence study
must vary physics step size while holding control cadence and inputs fixed.

Reproduce with `./Tools/Tests/test_physical_recovery.ps1 -Prefix PropulsionCore`.
The wrapper now checks process exit status, fresh solver-cadence output and
physical mission results, and keeps different matrices under separate prefixes.

The rendered hot-abort run `propulsion-ground-abort.log` passes all 12 ground
checks with the new engine bank. Reviewed ignition and safe-shutdown screenshots
show physical spool-up followed by zero thrust and extinguished flames while the
vehicle remains on its mount. Deluge persists after shutdown; the negative HUD
clock and inactive flight timeline remain correct. The wrapper restored the
user's settings after the run.

## Solver integration boundary

`RecoveryPhysicsAuditComponent`, enabled only with `-RecoveryPhysicsAudit`,
records `GetDeltaTime_Internal()` inside a real Chaos `TSimCallbackObject`.
The callback emits value-only output through Chaos' output queue. The game
thread consumes that queue and separately records actual flight-control steps.
It never reads a game-thread component from the physics callback.

The first 60 Hz post-change flight measured 82,713 solver callbacks versus
27,571 control steps: 5.555556 ms versus 16.666668 ms. Both integrated
459.516691 seconds. The configured maximum substep is 8.333333 ms, but that
maximum is not a fixed solver cadence. This observation prevents incorrectly
labelling the current controller as a fixed-step simulation.

Across all three scenarios, the measured game/solver pairs were 60/180 Hz,
30/150 Hz and 15/120 Hz. The different solver rates are a consequence of the
engine's frame subdivision; changing the maximum-step setting alone would not
decouple control from rendering.

[Epic's substepping documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-sub-stepping-in-unreal-engine)
explains the difference between frame forces and internal physics substeps.
The installed UE 5.8.2 source is decisive for the integration API:

- `Engine/Private/PhysicsEngine/Experimental/PhysScene_Chaos.cpp`:
  `AddCustomPhysics_AssumesLocked` directly executes the delegate with
  `MDeltaTime`; it does not install a per-substep force model.
- `Chaos/Public/Chaos/SimCallbackObject.h`: solver callbacks consume timestamped
  inputs and publish owned outputs across the game/physics thread boundary.
- `ChaosVehicles/Private/ChaosVehicleManager.cpp` and
  `ChaosVehicleManagerAsyncCallback.cpp`: register/unregister callbacks on the
  scene and evaluate vehicle physics from `OnPreSimulate_Internal`.

Next, move navigation, guidance, RCS and the engine-bank state behind that
boundary, with commands crossing into the solver and immutable snapshots/events
returning to presentation. Launch release, stage separation and rail contacts
must remain mechanical events. Repeating guidance several times against one
stale component transform would not meet requirement 71. Requirements 71–73,
83, 139 and 140 remain incomplete at this checkpoint.

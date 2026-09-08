# Booster dynamics in the Chaos solver — 8 September 2026

## Boundary

`RecoveryDynamicsModel` owns the booster fuel ledger, mass distribution, engine
valves, gimbals, grid fins, RCS valves, conditioning vents and deluge response.
It calculates aerodynamic loads and radial gravity from each solver step's body
position, orientation, velocity and angular velocity. It has no actor/world
access and never integrates or assigns a body pose.

`RecoveryPhysicsComponent` registers a Chaos `TSimCallbackObject`. Immutable
mission setup and held guidance/experiment commands enter through its input
queue. The physics callback reads `FRigidBodyHandle_Internal`, evaluates the
model, updates mass/inertia/centre of mass and applies the sum of individual
forces and their lever-arm moments. It subtracts the already enabled world
gravity from the reported total radial gravity. There is no extra attitude
torque or parallel game-thread booster force writer.

Timestamped outputs contain the model state and mission generation. The game
thread discards old-generation snapshots on reset. A post-physics component
publishes the current snapshot before presentation. HUD and Blueprint telemetry
properties are read projections; rendering cannot mutate the solver model.
Debug force vectors and mass-centre displays use these snapshots too.

Separation transports the initial rigid-stack velocity field to the two new
mass centres. Its momentum calculation now uses the analytical mass snapshot,
because game-thread Chaos mass properties are not the authority for the
physics-thread tank update. Separation remains a mechanical initial-state event.

## Validation

The editor build `solver-dynamics-validation-build.log` succeeds. Ten Unreal
unit tests pass in `Saved/Recovery/SolverDynamicsUnit/index.json`. Two added
model-level tests independently check the integrated engine force against the
analytic valve ODE at 30/60/120/240 Hz, fuel/exhaust momentum balance, passive
aerodynamic work, radial gravity direction, absence of actuator force after
shutdown, preservation of supplied kinematics and zero-time pause behavior.

The initial three 60 Hz flights pass for Nominal, Crosswind and Offset profiles,
under `SolverDynamicsFirst`. The nominal mission-result snapshot contains
82,866 model steps over 460.366691 s. Its final session audit contains 82,869
solver steps and 27,623 outer-control steps. The result is written before the
last frame; the final cadence audit is written after it.

The final `SolverDynamics-matrix.json` contains nine passing full flights. Each
of the nine final cadence audits records exactly the same step count and total
time for inner dynamics and the independent Chaos callback. The game/inner
cadences are approximately 15/120, 30/150 and 60/180 Hz, respectively.

| Nominal game updates/s | Apogee m | Main fuel used kg | Mission s | Landing burn s |
|---:|---:|---:|---:|---:|
| 15 | 97,254.61 | 3,566,602.10 | 395.07 | 68.40 |
| 30 | 97,215.24 | 3,567,805.86 | 398.53 | 72.00 |
| 60 | 97,212.13 | 3,568,982.37 | 400.38 | 73.90 |

All nine preserve both fitting contacts, finite engine shutdown, unpowered
coast, gimbal/force bounds and eight seconds of passive support. Maximum
propellant ledger error is `4.57e-8 kg`; maximum support drift is `0.05784 m`.
The largest separation angular-momentum relative mismatch is `0.000216`.
The matrix's source/module hashes are in `SolverDynamics-source.json`.

Compared with the earlier engine-bank-only nominal matrix, 15–60 Hz apogee
spread increased from approximately 22.8 to 42.5 m, and terminal duration
still varies. The change therefore establishes correct inner-loop scheduling,
**not improved whole-flight convergence**. Outer-control and solver cadences
still vary together in this matrix and must be isolated in the next study.

Three fresh unpowered contact fixtures pass: centred supports the booster,
incorrect heading does not acquire both rails, and a lateral impact produces
structural contact without capture. Reports are `ContactCentered.json`,
`ContactWrongHeading.json` and `ContactSideImpact.json`. These checks exercise
the new mission setup/reset path as well as the physical collision geometry.

The rendered terminal sequence passes ten checks (`SolverGroundLaunchSmoke`):
live home conditioning, cold hold on failure, resume, deluge, physical ignition
under restraint and release into ascent. That run exposed a PowerShell scalar
splat in the new wrapper's hot-abort argument; the fresh-report gate correctly
rejected it as hot-abort evidence. The wrapper now uses an explicit string array.

The corrected `SolverGroundAbortFinal` run passes all twelve hot-abort checks.
Fresh `Ignition.png` and `SafeShutdown.png` were inspected: the T-minus clock
remains, thrust reaches 80.88 MN on the mount, an injected engine failure aborts,
all displayed engine valves close, flames disappear, and cooling vapor persists.
This is lifecycle/force-to-render evidence, not a claim that smoke artistry or
terrain fidelity is complete.

The final rendered input audit passes all 69 checks (`SolverControls`): actual
key dispatch before/during ascent, experiment preservation, inspection/lab/menu
toggles, playback changes, physical pause and return-home conditioning. Fresh
`ForcesReady.png` and `ForcesAscent.png` were inspected. Engine force origins
follow the nozzle array and the gravity/mass-centre display follows the vehicle.
The rebase uses unit-scale physical transforms; inherited Blueprint artwork
scale is no longer applied to already scaled force-sample coordinates.

`solver-debug-frame-build.log` is the final successful build. A source-hash
comparison against the nine-flight manifest shows only the force-display
correction and input-audit screenshot additions changed afterward. Flight-model
source is identical to the tested matrix. The final module and source hashes
are retained in `SolverGroundAbortFinal-source.json`. The ground and input
wrappers restore the user's preferences; all validation processes exited.

## Remaining scope

Requirement 71 is **not complete**. Outer navigation, the ballistic predictor,
guidance, phase transitions, contact decisions, launch release and upper-stage
engine integration still run on the game thread. Inner dynamics now follows
every actual Chaos step, whose duration still varies with frame subdivision.
A successful catch matrix does not establish fixed-rate whole-flight control,
numerical convergence, flight-model accuracy or render-rate independence.

The next migration must move those outer-flight responsibilities and mechanical
event boundaries onto an explicit simulation clock, with interpolation confined
to presentation. It must retain bounded physical actuators and independently
verify separation/contact behavior. Requirements 72, 73, 83, 85, 139 and 140
remain open: the long landing burn, estimated aero/engine data, tank dynamics,
sensor model and complete replay/export state are not solved by this change.

## Primary references and reproduction

[Epic's substepping documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-sub-stepping-in-unreal-engine)
explains frame subdivision and delayed collision callbacks. The installed UE
5.8.2 `Chaos/SimCallbackObject.h`, `PhysicsProxy/SingleParticlePhysicsProxy.h`
and `ChaosVehicles/Private/ChaosVehicleManagerAsyncCallback.cpp` establish the
actual callback, input/output ownership and physics-thread handle APIs used.

- `Tools/Tests/test_physical_recovery.ps1 -Prefix SolverDynamics` runs all three
  scenarios at 60/30/15 game updates/s and checks actual model/solver cadence.
- `Tools/Tests/test_contact_fixtures.ps1` drops/impacts the unpowered booster in
  centred, incorrect-heading and side-impact cases.
- `Tools/Tests/test_ground_sequence.ps1 -HotAbort -Prefix SolverGroundAbortFinal`
  records a rendered hot abort, checks fresh reports/images and restores the
  user's exact settings bytes after the test. Images need visual inspection.

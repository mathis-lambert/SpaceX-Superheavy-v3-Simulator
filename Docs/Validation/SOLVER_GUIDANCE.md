# Flight guidance in the Chaos solver — 8 September 2026

Later development status: the [terminal guidance checkpoint](TERMINAL_GUIDANCE_CHECKPOINT.md)
records a subsequent landing regression. The successful flights below apply to
the earlier solver-guidance implementation, before that terminal experiment.

This follows the [inner-dynamics checkpoint](SOLVER_DYNAMICS.md). The full
[150-requirement program](../REALISM_PROGRAM.md) remains active. This wave
advances requirements 10, 71, 72, 122, 139 and 140; it does not complete them.

## Ownership and time

`FRecoveryGuidanceModel` now evaluates navigation, pitch programming, ballistic
prediction, boostback, entry, landing demands and flight phase transitions inside
the same Chaos callback as `FRecoveryDynamicsModel`. It reads current solver
kinematics and the previous actuator state, then issues bounded actuator demands
for the upcoming step. Neither model sets position, orientation or velocity.

`RecoveryNavigation::Evaluate` and `RecoveryAtmosphere::WindAt` are shared by the
flight computer and presentation telemetry. The mission director exchanges
commands and consumes snapshots; its former frame-driven guidance and ballistic
predictor have been removed. Predictor scheduling retains its fractional remainder.

Guidance starts after physical launch-mount release. A separation request waits
for confirmation that the second physical body exists before allowing boostback.
Operator abort overrides autonomous guidance. Explicit unpowered collision
fixtures retain their independent command ownership.

Ground sequencing, launch release, creation of the separated body, collision
callback delivery, tower-arm motion and upper-stage integration still use the
game thread. Thus this is not a fully fixed-rate simulation. Chaos divides each
game frame into actual substeps; their duration still changes with frame rate.

## Decision evidence and allocations

Flight events carry reason enums and numerical values in an inline array, rather
than allocating/copying `FString` messages every physics step. English labels are
resolved when the game thread consumes or exports an event. This removes the
new guidance-event string allocations; it does not establish that all runtime
allocations elsewhere in the project have been removed.

Reports now export `guidance_events`: phase, reason code, label, command endpoint
time, incoming sample time, sampled altitude and sampled mass. The two times
differ by one solver interval. These are not post-integration body samples.
Delayed delivery cannot replace those measurements with a later game-frame
altitude or mass. The first separation event still measures the attached stack;
the independently acknowledged boostback event measures the booster alone.

Each of the nine tested flights records seven ordered flight transitions. This
is a phase-decision timeline, not yet a complete record of sensor estimates,
control allocation, every failure decision, or a replay/debrief system.

## Unit checks

`solver-guidance-events-build.log` builds the implementation. The first unit run
had 11 passes and one failing reference: `PI` is single precision in UE, while
`DegreesToRadians(double)` uses `UE_DOUBLE_PI`. That introduced approximately
`1.18e-10 rad` error in a test expecting `1e-10` agreement. The test now uses
double precision without widening its tolerance or changing flight equations.

`solver-guidance-final-build.log` succeeds. The final report is
`Saved/Recovery/SolverGuidanceFinal-Unit/index.json`: twelve tests pass with no
warnings. The two guidance fixtures independently check:

- 1,800 physical samples at 120 Hz, grouped into 15/30/60 Hz frame batches,
  produce the same guidance demand and analytic pitch program.
- Navigation consumes the latest physical sample; mission time advances by
  actual substeps, including every substep inside a slow frame.
- Separation waits for mechanical acknowledgement; event measurements survive
  later samples; attached/separated mass is distinguished.
- Zero time does not advance guidance; abort closes commanded thrust; resetting
  and then aborting on the ground cannot start a flight clock.

`Tools/Tests/test_physics_models.ps1` now requires a fresh exported automation
report with executed, successful tests. A zero Unreal process exit code alone
does not prove passing assertions: the first failing run exited zero.

## Nine physical flights

`SolverGuidance-matrix.json` passes Nominal, Crosswind and Offset at 60, 30 and
15 game updates/s. `solver-guidance-comparison.json` retains aggregate values.
Every final audit has exactly equal dynamics/independent-solver step counts and
elapsed time. Flight guidance runs at approximately 180, 150 and 120 Hz,
respectively; ground preparation accounts for the earlier dynamics-only steps.

| Nominal game updates/s | Apogee m | Main fuel used kg | Mission s | Landing burn s |
|---:|---:|---:|---:|---:|
| 15 | 97,212.476 | 3,569,303.73 | 398.817 | 72.308 |
| 30 | 97,212.631 | 3,569,149.25 | 399.133 | 72.620 |
| 60 | 97,211.593 | 3,569,181.56 | 399.633 | 73.128 |

Nominal apogee spread decreases from the prior inner-dynamics wave's 42.48 m
to 1.04 m. Fuel-use spread decreases from approximately 2,380 kg to 154.5 kg.
These are scenario-specific improvements, not proof of whole-flight convergence.
Offset apogee still spans 25.00 m. Crosswind mission duration spans 5.67 s.

All nine retain two rail contacts, finite engine shutdown, bounded thrust and
gimbal, unpowered coast, mechanical separation and eight seconds of passive
support. Maximum support drift is 0.03941 m; maximum fuel balance residual is
`5.50e-8 kg`; maximum separation angular-momentum relative error is `0.000115`.

The landing burn is still 70.70–100.98 s. In the 60 Hz nominal trace, landing
ignition occurs near 4,499 m at T+318.51 s. Capture approach starts at 155 m at
T+357.79 s, with almost zero vertical speed; rail support is confirmed only at
T+391.63 s. The remaining terminal corridor/acquisition logic consumes substantial
time and fuel. Requirement 85 needs a redesigned feasible descent, not a shorter
timer or a forced vehicle pose.

## Scope of evidence

The matrix's source/module hashes are in `SolverGuidance-source.json`. Only the
test's pi reference and the new automation wrapper changed before the final
build; no flight-control equations were changed after the matrix.
`SolverGuidanceFinal-source.json` records the final unit build.

## Contact, launch abort and rendered interaction

All three unpowered contact fixtures pass on the final module. Centered reaches
both rails; WrongHeading reaches neither; SideImpact records 536 structural
contacts and only one rail. Guidance steps remain zero in these fixtures.
`success` here means the fixture's expected behavior passed, not that a
misaligned booster was captured. Fresh copies are in `SolverGuidanceContacts`.

`SolverGuidanceGroundAbort/result.json` passes twelve checks, from live home
conditioning through interlock hold, deluge, ignition and post-ignition abort.
The inspected `Ignition.png` shows 33 engines at 80.88 MN before mount release;
`SafeShutdown.png` shows zero thrust, the vehicle still on the mount and continuing
deluge. These establish the shutdown/render boundary, not final smoke quality.

`SolverGuidanceControls/result.json` passes 69 actual input/focus checks before
launch and during ascent: non-resetting keys, laboratory controls, camera cycling,
playback speed, pause and returning home. Fresh `ForcesReady.png` and
`ForcesAscent.png` were inspected; debug forces remain positioned on the vehicle
with a correctly placed mass-centre marker during ascent.

The rendered Crosswind mission passes at 1920×1080 with DLSS Quality, 15 Hz fixed
simulation frames and the Chase review. It records 7,228 frames, including 1,313
above 80 km, and finishes at T+421.817 s. Both rails support the booster with
engines off; final support drift is 2.140 mm. Across 121 captured Chase frames,
maximum relative offset change is `8.59e-12 cm` and maximum angular step is
`0.00000419 degrees`. This checks the captured interval, not every possible
manual camera transition. Starship's visible mesh/body alignment error is zero.

Fresh reports and phase images are in `SolverGuidanceRendered`. `COAST.png`,
`CAPTURE.png` and `SECURED.png` were inspected. Remaining coarse ground textures,
repetitive cloud coverage and plume quality are visible; this wave adds no new
visual-fidelity or GPU-performance claim. Stale unrelated review images were
excluded from the copied evidence directory.

The final runtime/source manifest is `SolverGuidanceRendered-source.json`.
The input/render wrapper restores the original settings bytes in `finally`;
post-test settings text matches the manifest. All validation processes exited.

## Terminal-descent research and next implementation

The nominal trace contains about 48.3 s of terminal-phase samples below 200 m,
including 28.6 s with vertical speed magnitude below 1 m/s. These are sampled
phase durations, including the final shutdown/contact interval. They isolate
the corridor/settling delay from the earlier atmospheric braking problem.
Requiring positive measured engine thrust reduces the slow-descent interval
to approximately 23.6 s, still a substantial target for the next guidance work.

NASA Langley's 2024 [successive-convexification study with time-varying mass
properties](https://ntrs.nasa.gov/api/citations/20230018515/downloads/SciTech2024_Charts_updated.pdf)
considers coupled translation/attitude, changing mass properties, thrust and tilt
limits, and feasibility checks. It reports control-history differences when mass
properties vary. JPL's [G-FOLD flight-test account](https://www.jpl.nasa.gov/news/jpl-masten-testing-new-precision-landing-software/)
provides a real demonstration of onboard divert planning. Neither source supplies
a Super Heavy flight-computer implementation or calibrated vehicle parameters.

The implementation direction inferred from these references and this project's
trace is a bounded, receding-horizon descent planner with a common arrival time
for position, velocity and fitting alignment. Candidate trajectories must account
for actual available engines, valve response, gimbal/attitude rates, fuel reserve
and tower clearance. Feasibility failure must be reported and trigger a physical
alternative; solver slack must never become an additional force applied to the
vehicle. The current fixed-height waiting corridor should be replaced only after
that planner is verified on offset, wind and degraded-actuator cases. This
planner is not implemented by the present scheduling migration.

## Primary references and reproduction

[Epic's substepping documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-sub-stepping-in-unreal-engine)
explains frame subdivision and delayed collision callbacks. The installed UE
5.8.2 `Chaos/SimCallbackObject.h` defines the callback time and input/output
ownership. `Math/UnrealMathUtility.h` defines the precision-specific angle
conversion and smoothstep functions used when checking the analytical fixture.

- `Tools/Tests/test_physics_models.ps1 -Prefix SolverGuidanceFinal`
- `Tools/Tests/test_physical_recovery.ps1 -Prefix SolverGuidance`
- `Tools/Tests/test_contact_fixtures.ps1`
- `Tools/Tests/test_ground_sequence.ps1 -HotAbort -Prefix SolverGuidanceGroundAbort`

Requirements 71–73 remain open: a controlled solver-step convergence series,
mechanical event timing, upper-stage integration and independent physical
reference cases are still necessary. Estimated aerodynamics, sensor truth,
engine restart limits, tank dynamics and complete replay state remain separate
unfinished requirements.

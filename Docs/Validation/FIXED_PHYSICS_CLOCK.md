# Fixed physical clock and atomic separation — 9 September 2026

The previous solver callback removed game-thread force calculations from the
booster, but Chaos still divided each game frame into variable substeps. The
measured physical rates were 180/150/120 Hz at 60/30/15 game updates per second.
Starship propulsion was still integrated once per game frame. This wave moves
both stages onto a fixed physical clock and removes interpolated render state
from the mechanical stage-separation transfer.

## Scheduling and ownership

`DefaultEngine.ini` enables asynchronous Chaos at 120 Hz and disables both
substepping switches. `MaxPhysicsDeltaTime=0` lets the fixed-step accumulator
consume the complete supplied game interval. `p.AsyncPhysicsBlockMode=1` waits
for the interpolation bracket and does not use the mode that drops queued
physics steps. A slow frame changes how many fixed intervals run, not their size.
Zero-duration flush callbacks apply no cached forces.

This follows [Epic's fixed-step settings](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-settings-in-the-unreal-engine-project-settings)
and the installed 5.8.2 implementation in `PhysicsCore/Private/ChaosScene.cpp`
and `Chaos/Framework/PhysicsSolverBase.cpp`. The two substepping switches are not
combined with async ticking; Epic also documents the conflicting configuration
in [UE-185525](https://issues.unrealengine.com/issue/UE-185525).

`FRecoveryUpperStageModel` owns Starship fuel, valve state, mass, inertia and force
samples. Its six nozzles use the integrated opening-valve impulse and a finite
fuel budget. End thrust drives presentation; mean force over the physical step
drives Chaos. Gravity and passive air loads use the current solver body. The
former `TickUpperStage` and its game-frame force application have been removed.
The existing estimated six-engine, symmetric fixed-direction model is retained;
this does not add orbital guidance or detailed Starship tanks.

The common solver adapter reads kinematics, updates mass properties and applies
force/lever-arm sums for both stages. Numeric setup and output snapshots are
generation tagged. The game thread consumes them for the HUD, artwork and
reports. The stage model does not read actors, components or the world.

## Separation regression and correction

The first async probe passed the nine traditional flights and 59 FPS, but failed
at 144 FPS after propellant exhaustion. The old separation code combined an
interpolated component transform with physics velocities and a separate mass
snapshot. This was not a valid common rigid-stack state.

Proxy allocation remains on the game thread. The actual split now runs once in
the physics callback, using the live incoming rigid body. `RecoverySeparation`
transports its velocity field to both new centres of mass, retains orientation
and angular velocity, and records linear and angular momentum residuals. The
guide acknowledges this physical split before boostback. These state assignments
create two bodies from one assembly; they are never used by flight guidance or
capture to correct the vehicle's pose or velocity.

## Verification

The Development Editor build passes and all 20 Unreal model tests pass with no
warnings. The three added tests cover upper-stage valve impulse/fuel accounting,
passive coast loads and a rotated, translating rigid-stack separation velocity
field. Four Python evidence fixtures pass, including rejection of trajectories
that diverge even though every catch succeeds.

Three unpowered contact fixtures pass. In the centred case, signed vertical
rail reaction totals 14,245,247 N s after 4.975 physical seconds for a 292,000 kg
body, closing the weight-impulse check within its independent 2% bound. Incorrect
heading and lateral impact do not falsely count as fitting support.

The effective display-stall flight passes with solver, booster, guidance and
upper stage all retaining the 1/120 s interval. Both rails support the body,
engines are off, the frontal ingress audit passes, and eight-second support
drift is 5.77 cm. The repeated 200 ms frame is verified in measured game timing.

The nominal refinement runs at 120, 240 and 480 physical Hz all catch correctly.
Post-capture drift decreases from 5.681 cm to 2.871 cm to 1.457 cm. During ascent,
matching-time position RMS differences decrease from 2.841 to 1.417 m and speed
RMS differences from 0.03594 to 0.01801 m/s. **Whole-recovery convergence fails:**
its position differences increase from 4.404 to 38.279 m and speed differences
from 0.06673 to 0.51335 m/s. The analyzer returns failure. A successful capture
and improving contact drift do not justify marking requirement 72 complete.
Event timing and the discrete terminal planner still need focused refinement.

All nine complete flights pass at 60/30/15 game updates per second for nominal,
crosswind and 5% additional dry mass. Physics remains 120 Hz in each. Every run
enters from the front, has zero structural hits and finishes with both physical
supports loaded and propulsion off. Minimum mast-front clearance is 11.859 m;
minimum final-corridor margin is 0.366 m. Eight-second support drift spans
4.676–5.838 cm. Additional nominal 59 and 144 FPS runs also pass, covering
non-divisor and faster-than-physics game cadences.

Maximum absolute booster fuel-ledger error is 3.40e-8 kg and maximum relative
separation angular-momentum residual is 5.85e-8 in the nine-flight matrix.
Landing burns remain 69.5–81.7 seconds; this wave does not optimize them.

Evidence under `Saved/Recovery`: `FixedPhysicalVerified-Unit/index.json`,
`FixedPhysicalContact{Centered,WrongHeading,SideImpact}.json`,
`FixedPhysicalJitter_Nominal_60.json`, `FixedPhysicalMatrix-matrix.json`,
`FixedPhysicalDisplay-matrix.json`, the 240/480 Hz
`FixedPhysicalRefine{240,480}_Nominal_60.json` and their CSV/cadence reports,
`FixedPhysical-matrix-metrics.json`, and `fixed-physical-convergence.json`.
The first attempted refinement run is deliberately excluded: the harness split
a scalar PowerShell argument into characters. The actual clock gate rejected
it. A typed argument array fixes the fixture and fresh measured runs confirm
240/480 Hz. Earlier jitter prototypes are likewise excluded when their requested
display stalls did not appear in measured time.

Each source manifest covers 377 project, configuration, content, tool and DLL
files. The runtime hashes agree throughout these final checks. The first three
manifests differ only in the subsequently corrected test-runner argument array;
later manifests have no differences. See `FixedPhysical-source-verification.json`.
Rendered verification is recorded below after completion.

## Support observations

The first effective 200 ms display-stall fixture exposed a support-observation
bug: the booster rested on both rails with its engines off, but the old 200 ms
game-frame hit-event grace period repeatedly expired. The mission timed out.
The collision observer now visits Chaos's solved contact manifolds after every
physical step. It accepts upward, resolved contacts inside the fitting volumes
against each known rail, and supplies the resulting support mask to guidance
on the next physical step. No artificial support force, position correction or
attachment is introduced. Game-frame structural-hit messages remain diagnostic.

The observed reaction follows Chaos's own `NetImpulse + NetPushOut / Dt`
reporting convention. The position-solver term is an equivalent reaction
impulse, not an extra force applied by the observer. Vectors are summed for each
rail before accumulating their magnitude; signed vertical impulse is exported
separately. Cached sleeping-manifold impulses are not accumulated again.
An independent unpowered drop fixture checks that integrated upward reaction
balances integrated body weight within 2%, in addition to checking that a wrong
heading and a side impact cannot count as successful support.

## Refinement and display-stall fixtures

`-RecoveryPhysicsHz=240` or `480` refines the solver interval for convergence
runs. The CSV's `solver_sample_time_s` and `solver_*` kinematics are the incoming
physical sample, separate from the later actuator endpoint and interpolated
render telemetry. `analyze_physics_convergence.py` compares matching physical
times during ascent and recovery, requires identical game cadences and declining
position/velocity RMS differences as the physical rate doubles. Its independent
fixtures reject successful captures with diverging trajectories.

The cadence audit also supports `-RecoveryJitterClock`: a repeatable sequence of
144/59/30/144 Hz intervals, a 200 ms stall and a 60 Hz interval. This only operates
in explicitly requested fixed-time audit runs and restores the original game
clock on exit. The audit checks actual solver, booster, guidance and upper-stage
step durations, plus accounted game/physics elapsed time.

## Remaining boundaries

Ground sequencing, operator-command delivery, proxy creation, structural-hit
diagnostics and the kinematic tower still involve game frames. Fixed flight-model
cadence is not a claim of bit-identical complete missions at every display rate.
Tower actuator dynamics, sensor timing and a complete replay state remain open.
The original [150-item programme](../REALISM_PROGRAM.md) remains active.

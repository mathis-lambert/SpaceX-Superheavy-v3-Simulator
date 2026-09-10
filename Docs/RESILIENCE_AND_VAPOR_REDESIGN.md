# Alpha.6 audit: resilient flight decisions and continuous vapor

10 September 2026. Audited source `794a671`; packaged game source `646ce82`.
This document proposes the next implementation. It does not describe completed
features. Existing user-session reports and a read-only Blender inspection are
preserved in [Research/Resilience](Research/Resilience/audit-evidence.json).

## Observed failures

The current local game log contains three completed mission reports:

| Flight | Outcome | Landing thrust | Terminal plans rejected / attempted |
|---|---|---:|---:|
| 1 | Physical capture | 31.55 s | 21 / 24 |
| 2 | Boostback depleted landing reserve | 0 s | 0 / 0 |
| 3 | Main propellant exhausted | 189.61 s | 932 / 932 |

Flight 3 never found an accepted terminal trajectory. The fallback continued to
target the tower until fuel exhaustion, at approximately 195 m altitude. The
report records 44.19 seconds of fin control and a landing ignition at 2,413 m;
it therefore does not support the conclusion that neither fins nor engines ever
operated. Their intervention did not produce a viable recovery.

That flight also combined engine index 8 failure, fin index 0 jam, attitude gain
changes and wind scale 3. Recorded RCS outages lasted 2.992, 11.300, 6.917 and
3.500 **simulation seconds**. Accelerated playback makes manual click durations
different from simulation durations. An isolated one-second RCS failure has not
been replayed in this audit, and tower reachability has not been independently
established. Those must become separate, reproducible tests.

## Confirmed architectural limitations

1. `RecoveryGuidanceModel.cpp` sets engine count to zero throughout Coast/Entry.
   Only the landing-ignition condition exits this policy; there is no midcourse
   correction burn or return to a recovery boostback mode.
2. The ballistic predictor runs a point-mass, unpowered forecast with assumed
   tail-first drag. It omits the evolving attitude, commanded control allocation
   and alternative powered trajectories. Large attitude errors can invalidate
   this forecast precisely when recovery matters most.
3. `RecoveryLandingPrediction.cpp` evaluates a vertical braking trajectory with
   a fixed current up-projection. Nonpositive up-projection or insufficient core
   braking returns infeasible. The phase transition requires feasibility; there
   is no separate maneuver to recover attitude and reassess the landing choice.
4. Terminal planning begins below 1,800 m and 180 m/s. Its sampled polynomial
   family has one destination: the capture fittings. If candidates fail, energy
   braking still aims at that same target. A lost-tracking replan can also fail
   while leaving the old accepted plan flagged usable; capability-loss handling
   invalidates it in a narrower case. Neither path supplies a certified fallback.
5. `RecoveryDynamicsModel.cpp::AttitudeMoment` counts nominal RCS torque whenever
   RCS fuel remains, ignoring the disabled flag and pressure blend. Its scalar
   authority estimate also does not represent individual-axis actuator limits.
6. RCS permission depends on **requested engine count**, rather than delivered
   engine control authority. Fin moment demand drops to 10% when engine count is
   positive. A command to ignite is therefore treated as a change in available
   control before that authority has necessarily arrived.
7. Fin demand uses a fixed three-fin allocation; jamming one fin does not
   redistribute its missing contribution across healthy fins and other controls.
8. Reserve depletion and envelope failure enter Aborted, suppressing propulsion
   and active fin control. A failed tower mission is not distinguished from an
   operator command to terminate propulsion or a controlled alternate landing.
9. The flight matrix covers Nominal, Crosswind and Offset. The world audit checks
   fault injection, valve response and restoration, but does not fly repeated
   outages through recovery. Existing green tests do not establish fault resilience.

The current terminal planner does consider fuel and searches different durations.
This is not simply a fuel-minimization setting that should be turned up. More
fuel burned at the wrong time or attitude does not guarantee a reachable tower.

## Why the vapor looks stepped and boxed

The original domain is 20 × 16 × 12 m, 64 frames at 24 fps. Inspection of the saved
Blender file confirms **all six collision borders are open** and the adaptive
domain is disabled. It is not a closed-wall simulation.

The runtime multiplies that domain by `4.6 + age * .18`, reads `age * 8`, and clamps
at frame 63 for a twelve-second lifetime. Thus the shape receives at most eight
new source frames per second and freezes after 7.875 s for the remaining 4.125 s.
Translation and expansion continue, which can disguise the freeze in screenshots.

The installed UE5.8 component requests one frame. Its SVT helper casts the float
frame index to an integer; fractional `SetFrame` values do not interpolate density.
`bPlaying=false` also means no valid playback rate is supplied to the streaming
request. Streaming can worsen stepping, but its contribution was not measured here.

The material samples density directly without a spatial extinction margin at the
domain boundary. Large scaling, a bounded cache and increased extinction can
expose rectangular clipping surfaces. Exact per-voxel boundary density and
multi-volume compositing artifacts still need a motion capture/isolation test.

The previous checks established visible volume contribution and bounded instance
counts. They did not establish temporal smoothness. Still screenshots are
insufficient acceptance evidence for this effect.

## Target architecture

Keep force integration, mass accounting, actual actuator response and mechanical
contacts as the reference plant. Replace phase-specific recovery decisions with
an explicit mission supervisor, forecasts and actuator-aware control allocation.
Phases remain useful labels for presentation and hardware sequencing.

```mermaid
flowchart LR
    P[Physical vehicle] --> S[State and actuator health]
    S --> F[Candidate trajectory forecasts]
    F --> M[Mission objective and recovery decision]
    M --> G[Accepted guidance reference]
    G --> A[Available actuator allocation]
    S --> A
    A --> P
    F --> U[Flight computer view]
    M --> U
    A --> U
```

### 1. Decide while recovery is still possible

- Evaluate tower return and designated offshore alternatives before terminal
  braking. Track time and fuel needed to restore attitude as part of reachability.
- Forecast a conservative set of reachable positions and arrival velocities,
  with uncertainty from wind, aerodynamic error, mass and actuator response.
  Sampled reachability is an estimate, not a proof of global infeasibility.
- Replan on significant tracking error, control saturation or health changes.
  Allow corrective burns when an unpowered trajectory will lose recovery margin.
- Give every accepted plan an age, input-state timestamp, validity envelope and
  executable fallback. Do not keep following an invalid trajectory indefinitely.
- Choose objectives in order: avoid protected ground areas and structures;
  reach an acceptable recovery corridor; meet contact limits; retain reserves;
  reduce unnecessary fuel. Make reserve policy state-dependent, not a magic amount.
- A failed solve is not evidence that no solution exists. Distinguish numerical
  failure, exhausted search budget, expired input state and constraint violations.

Begin with a bounded translation planner and explicit attitude feasibility checks,
then evaluate a six-degree-of-freedom receding-horizon formulation against the same
force model. Run optimization outside the physics callback. Timestamp its results,
bound computation time and reject stale or invalid outputs. The 120 Hz inner
controller must remain operational when a planning result is late.

NASA's work on [successive convexification with changing mass properties](https://ntrs.nasa.gov/citations/20230018515)
is a useful algorithmic reference. It is not a ready-made atmospheric Starship
controller, nor evidence about SpaceX's proprietary flight software.

### 2. Use the controls that are actually available

- Maintain a health/authority description per engine, gimbal, fin and reaction
  nozzle: availability, measured response, bounds, rates and remaining resources.
- Allocate requested forces and moments jointly within those limits. Feed the
  achievable result and saturation residual back into attitude control and planning.
- Account for valve opening and thrust buildup before reducing other controls.
  Hardware interlocks still apply; simultaneous RCS/engine use is not assumed
  universally permitted by the real vehicle.
- Reallocate after a fin jam or engine loss; stabilize angular velocity before
  pursuing aggressive trajectory correction when that is physically necessary.
- Retain command/measurement separation. Injected faults first provide a known
  health state; hidden failures and sensor estimation are a subsequent extension.

### 3. Separate a failed catch from abandoning the vehicle

Introduce explicit outcomes: `Tower recovery`, `Controlled ditching`,
`Unrecoverable descent`, and `Operator termination`. Tower recovery permission
depends on the vehicle **and** tower state. Offshore alternatives use authored
exclusion zones and a reachable approach corridor; a booster without landing gear
does not magically perform a recoverable land touchdown there.

Use hysteresis and minimum commitment margins to avoid switching targets every
frame. Keep attitude and impact-energy control active when a catch is unavailable,
as long as actual hardware, fuel and operational limits allow it. Preserve the
operator's explicit termination command as a separate action.

The objective of minimizing arrival error under limited propellant has a public
[NASA/JPL reference](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/20120001230.pdf).
Our site selection and ground-protection rules would remain simulator design
choices, not a reproduction of an operational flight-safety system.

### 4. Make the flight computer inspectable

Add a visible **Flight computer** button and evolve the existing `L` Flight Lab
entry into four compact views: Overview, Trajectory, Systems, Events. Keep the
main flight HUD sparse.

- Show measured/predicted motion, the accepted target path, impact estimate,
  estimated reachable area, protected zones and scheduled burn intervals in 3D.
- Display current objective, decision reason, target validity, time to action,
  fuel required/available/reserved, braking margin and attitude-recovery margin.
- Show requested versus delivered thrust/moment, actuator saturation, unavailable
  hardware and active interlocks. A command is not a delivered force.
- Persist each rejected candidate's limiting constraint and solver status.
  Example: `No accepted tower path — attitude-rate limit`, with expandable detail.
- Expose timestamp, age and confidence of the forecast. The UI reads immutable
  telemetry snapshots; it never edits the physical pose or runs the planner.
- Provide timed fault buttons such as `RCS outage / 1 simulation second`, plus a
  timeline for combined scenarios. Pause and accelerated playback remain explicit.

### 5. Rebuild vapor for motion

First remove the known eight-frame stepping, terminal freeze and hard boundary
exposure. Any revised cache must cover its intended lifetime at its authored time
scale. Use explicit temporal interpolation and validate it on the actual renderer;
do not assume a float frame index or a generic Niagara cache performs this work.

Evaluate a hybrid prototype: bounded local 3D gas simulation around impingement
and the tower, a cheaper advected ground layer for broad transport, and continuous
distant wisps. The transition must hide finite domains, preserve movement through
their edges and respond to wind and delivered deluge/propulsion flow. Keep cold
conditioning vapor separate from heated ground vapor.

For cached fields, test zero-density padding, absorbing edge bands and velocity-
aware interpolation. Simple density blending can ghost fast-moving structures;
it is a comparison candidate, not the final quality criterion. Avoid indefinitely
enlarging a small cache to fill the site.

Epic documents [two-frame SVT interpolation](https://dev.epicgames.com/documentation/unreal-engine/sparse-volume-textures-in-unreal-engine?lang=en-US)
and [Niagara gas boundary controls](https://dev.epicgames.com/documentation/unreal-engine/niagara-fluids-reference-in-unreal-engine).
Neither feature guarantees a performant complete plume. Compare the prototype
against alpha.6 with matched views, GPU timings, streaming statistics and video.

## Implementation sequence and acceptance

| Batch | Deliverable | Required evidence |
|---|---|---|
| A | Fault scheduler, decision log, regression baselines; vapor playback/boundary prototype | Replay captured session plus isolated 1 s outages; motion review of one volume and the full field |
| B | Health-aware control allocation and validity monitoring | Fin/engine/RCS faults with command-versus-delivery traces; no imaginary control authority |
| C | Midcourse replanning and alternate objectives | Recovery within independently checked reachable cases; controlled alternate choice when appropriate |
| D | Flight computer views and guided experiments | Readable decisions and 3D paths, no pose writes, timed faults unaffected by display cadence |
| E | Continuous hybrid vapor and performance tuning | No exposed box surfaces or frozen tails in representative moving views; measured GPU/VRAM/streaming cost |

Keep the current successful returns as regression baselines. Add outages during
separation, coast and entry; permanent RCS loss; individual fin jams; engine and
gimbal failures; wind changes; low fuel; bad approach heading; and an unavailable
tower. Reproduce each at multiple game cadences and with accelerated playback.
Judge physically feasible captures, controlled alternatives and unavoidable losses
separately. A red catch indicator can be the correct outcome of a successful
emergency decision.

Use repeatable seeds and recorded commands, tolerances for Chaos replay, and
headless scenario sweeps. Record the first decision that loses recovery margin,
not just the final crash. Compare predicted and measured future states to catch
model mismatch before optimizing controller gains around it.

## Further gains after the core change

An event-synchronized mission replay can explain why an objective changed and
compare alternative controllers from the same initial conditions. A parallel
advisory planner can be evaluated without controlling the vehicle until it beats
the baseline on recovery quality and computation budget. A later advanced mode
can add imperfect sensors, estimation and uncertain actuator failures. These
features build on observable decisions and reproducible experiments rather than
adding more unconnected HUD numbers.

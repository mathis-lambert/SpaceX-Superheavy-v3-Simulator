# Terminal reference tracking — 8 September 2026

This follows the failing [development checkpoint](TERMINAL_GUIDANCE_CHECKPOINT.md).
It advances requirements 10, 85, 86, 95, 103, 122, 139, 140 and 147 in the
[150-requirement program](../REALISM_PROGRAM.md). None is declared complete by
this report. Efficient terminal guidance, complete actuator prediction and the
physical tower mechanism remain unfinished.

## Flight-computer changes

The terminal planner searches a family of quintic centre-of-mass trajectories.
It retains incoming position, velocity and acceleration, requests a slow arrival
at the catch fittings, and checks sampled thrust, tilt, turn rate, fuel budget
and tower/arm clearance. Its future point-mass force estimate includes the
coupling between thrust direction and body aerodynamic normal load. It is an
estimated prediction; only the Chaos dynamics model moves the actual booster.

The initial actuator sample can be in a transition between engine banks or just
outside the guidance command envelope. Command limits are checked on future
samples, rather than requiring the already observed transient to change
retroactively. Finite valves, gimbal travel/rate, fuel limits and moments remain
independently enforced by the force model.

The equilibrium inversion now continues from the nearby body/previous thrust
direction. Starting every solve vertically could choose another aerodynamic
root near loss of lateral authority. Failed inversions return an explicit false
result with zero output; a large numeric sentinel can no longer become a tiny
upward command after tilt clamping. Incoming net acceleration includes fins,
vents and radial gravity as well as body drag and engine force.

Before a feasible transfer exists, braking leaves time for lateral displacement
and velocity removal. This replaces the old vertical-only stopping curve and
the earlier fixed hover shelf. The accepted trajectory is then retained instead
of repeatedly moving its arrival deadline. Search resumes for expired references,
tracking loss or a significant loss of central-engine capability. Routine
well-tracked steps no longer run another trajectory search.

Tracking uses separate lateral and vertical feedback gains. Valve anticipation
and a reference angular-rate demand account for actuator response. The latter
enters the bounded moment controller and produces physical gimbal/fin forces;
it never assigns body angular velocity. Close to the rails, the position demand
accounts for the actual fitting geometry and moving mass centre. Predicted
arrival geometry uses an arrival equilibrium, not the current flight attitude.

## Physical support

The capture phase prepares the arms while the booster is still approaching.
Clearance limits their closure around the occupied body. Once a fitting carries
load, the arm command is retained: the former feedback opened the rails in
response to settling motion, withdrawing a support during crosswind capture.
There is no late forced command to fully close already loaded rails.

Engine shutdown follows verified fitting contact and the existing position,
velocity and tilt gates. `SECURED` now requires both supports, speed below
0.05 m/s and angular speed below 0.001 rad/s continuously for one second.
The following eight seconds still verify passive support with engines off.
This changes the declaration of settled support, not the body's motion.

Tower arms remain kinematic components commanded by the game thread. Retaining
their command is not a model of real actuators, hinges, cable elasticity or rail
dampers; those remain requirements 97–103.

## Evidence

- `terminal-settled-build.log`: successful editor build.
- `TerminalSettled-Unit/index.json`: 15 passing tests, no warnings. New cases
  replay a measured terminal input, reject insufficient reserve, check retained
  search diagnostics and verify that angular-rate requests generate real engine
  moments without changing supplied kinematics or producing torque without fuel.
- `TerminalTrackingFinal-matrix.json`: all nine Nominal/Crosswind/Offset flights
  pass at 60, 30 and 15 game updates per second. Maximum reference-tracking
  error is 1.7345 m; maximum drift during settled support is 0.04294 m.
  Dynamics and independently measured solver counts/times agree exactly in all
  nine audits. Maximum fuel-ledger residual is `5.78e-8 kg`.
- `TerminalTrackingFinal-source.json` identifies the tested sources and binary.
  The rendered check has a separate `TerminalTrackingRendered-source.json`.
- Fresh Centered, WrongHeading and SideImpact contact fixtures pass. Centered
  has two supports and no structural contacts; WrongHeading has neither support;
  SideImpact records 536 structural contacts and one support. The latter two
  pass their expected collision checks, not successful-capture criteria.
- The rendered Crosswind flight passes with 7,209 observed frames, including
  1,313 above 80 km. The 121 Chase frames after capture show maximum anchor-offset
  step `7.28e-12 cm` and angle step `2.42e-6 degrees`. Rendered Starship/physical-body
  position error is zero in the 719 visible separated-stage frames.
- `TerminalTrackingRendered/COAST.png`, `CAPTURE.png` and `SECURED.png` were
  inspected. Actual settings were 1920x1080, DLSS at 66.7%, cloud mode 3,
  hardware Lumen enabled and solar hour 17. Preferences were restored, every
  source/content/config/tool/DLL hash matches the evidence manifest, and the
  fresh rendered log contains no material compilation failure or fatal error.

`Saved/Recovery/terminal-tracking-validation-summary.json` retains aggregate
values. The visual checks confirm flight presentation and support; they also
show blurred terrain imagery, sparse site detail and repetitive clouds. Those
world/rendering requirements remain open. This wave does not claim new FPS or
visual-quality improvements.

The preceding `TerminalReferenceMatrix` is deliberately retained as failing
evidence: all three crosswind cadences lost support when the rails withdrew.
`TerminalSupport_Crosswind_60` then supported the booster but failed the unchanged
0.25 m drift gate because `SECURED` was declared during residual rocking. The
stricter settlement gate passes that same drift limit; the limit was not widened.

## Measurement definitions and remaining limits

`landing_burn_seconds` now counts physical thrust above 1 N during the landing
and capture phases, including valve shutdown. Older reports counted the whole
phase, including engine-off settling. Compare older/newer powered duration using
their sampled `thrust_n` CSV column, not those differently defined JSON counters.
`terminal_planned_s` and `terminal_braking_s` count guidance modes, not exhaust
impulse. Fuel remains measured from the actual engine force integration.

At 60 Hz, sampled powered near-hover time below 200 m (`abs(vz) < 1 m/s`) changes
from 23.6 to 2.6 seconds for Nominal and from 33.5 to 4.1 seconds for Crosswind.
This is not a claim that total burn or fuel consumption decreased. Main fuel
consumption is approximately 3,576,165 kg nominal and 3,583,734 kg crosswind,
both higher than the preceding solver-guidance cases. Requirement 85 remains
open. The successful nominal run needs 79 planner searches, versus 298 in the
preceding fixed-reference implementation; this is a count, not an FPS benchmark.

The planner samples 24 intervals and uses simplified geometry, frozen prediction
wind and a point-mass aerodynamic model. It does not prove continuous collision
clearance, model all actuator transients in its prediction, solve a six-degree-
of-freedom optimal-control problem, or establish robustness to arbitrary failures.
Ground events, tower motion and upper-stage updates retain the previously
documented game-frame dependence. Real-world vehicle fidelity and optimal fuel
use are not established by successful scenario captures.

The distinction between polynomial guidance and constrained coupled descent is
documented in [NASA Langley's powered-descent presentation](https://ntrs.nasa.gov/api/citations/20230018515/downloads/SciTech2024_Charts_updated.pdf).
Its treatment of constraints and changing mass properties informs the remaining
work; this implementation is not NASA's successive-convexification algorithm or
SpaceX flight software.

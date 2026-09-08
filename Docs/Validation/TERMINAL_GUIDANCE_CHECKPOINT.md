# Terminal guidance development checkpoint — 8 September 2026

This is a work-in-progress source checkpoint, not a validated simulator release.
It includes the solver-guidance changes documented in [SOLVER_GUIDANCE.md](SOLVER_GUIDANCE.md)
and the subsequent terminal-transfer experiment. The nine successful flights in
that earlier report do not validate this newer landing implementation.

## Current evidence

- `Saved/Recovery/terminal-opening-build.log`: editor build succeeded.
- `Saved/Recovery/TerminalOpening-Unit/index.json`: 13 tests passed, no warnings.
- `Saved/Recovery/TerminalOpening-matrix.json`: Nominal, Crosswind and Offset
  all failed at 60 game updates per second before successful capture.
- The nominal run rejected all 126 terminal plans and used fallback braking
  throughout its 34.622-second landing burn. It aborted with `Flight envelope
  exceeded`, two structural contacts, neither rail supporting the booster,
  altitude 31.719 m and horizontal target error 41.505 m.

The shorter burn in this failed run is not an improvement demonstrated by a
successful landing. The point-mass quintic planner passes its isolated reachable
fixture but has not established feasibility for actual incoming flight states.
Its sampled clearance and actuator checks are estimates, not a full six-degree-
of-freedom trajectory optimization or a guarantee of collision avoidance.

## Required follow-up

Diagnose real-flight plan rejection, correct the planning-state and feasibility
handling, and obtain physical capture without restoring an artificial body pose.
Then repeat varied-cadence flights, collision fixtures and rendered capture
checks against the exact revised sources. Requirement 85 and the complete
[realism program](../REALISM_PROGRAM.md) remain open.

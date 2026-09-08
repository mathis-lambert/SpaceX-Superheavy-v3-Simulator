# Front approach to the catch tower

The previous capture controller could succeed after crossing behind the catch
axis and returning over the mast. Physical support alone did not validate the
route. The new approach policy reserves braking distance in front of the tower,
rejects paths outside the opening corridor, and audits the actual rigid body
throughout recovery.

## Reference and coordinate convention

The visual reference is the [official Flight 5 film](https://www.youtube.com/watch?v=hI9HQfCAw64&t=128s),
inspected around 2:07–2:20. The booster presents itself on the open side of the
arms, corrects attitude, and lowers its fittings onto the rails. The film is a
qualitative reference; it does not supply the guidance coefficients below.

Tower-local +X points out through the arm opening, +Y crosses the two rails.
The current catch base is 24 m ahead of the mast. Booster heading is still
90 degrees relative to the tower so the two modeled fittings align with the
opposed rails. This is distinct from the direction of approach.

## Implementation

- Boostback and aerodynamic entry target a point in front of the opening.
  `FrontReturnOffsetM` defaults to 1,400 m. An upwind correction uses the wind
  at landing ignition altitude and `LandingWindLeadS`, default 18 s. These are
  declared engineering estimates in the vehicle profile, not published flight
  specifications. Neither changes the body state.
- Terminal prediction searches 1–75 s durations in 0.5 s increments. Its base
  polynomial retains measured position, velocity and net acceleration and ends
  at the contact target with the requested descent velocity and zero net
  acceleration.
- Additional crossrange curves let lateral alignment happen earlier. The term
  `64 s^3 (1-s)^3 * CrossrangeCorrectionM` leaves position, velocity and
  acceleration unchanged at both endpoints. Bias values 0, 0.75 and 1.5 are
  tested against the same force, fuel, attitude and geometry limits.
- After the first actuator-transition interval, predicted turning rate is
  limited to 0.05 rad/s. The initial interval retains the existing 0.17 rad/s
  bound. Position and velocity feedback request real force and torque; the
  lateral gains are 0.035/s² and 0.20/s. No position or velocity setter was
  introduced.
- Candidate geometry checks the entire conservative body envelope against the
  mast's vertical projection, even above mast height. Below 350 m above the
  catch base, the fitting midpoint must enter a frontal corridor: longitudinal
  offset at least -0.6 m; crossrange at most `2 + 0.25 * max(front, 0)` metres.
  This leaves manoeuvring space inside the open arms. The separate 0.5 m
  predicted fitting-alignment check at arrival remains in force.
- The actual Chaos body is audited from separation through the eight-second
  support check. Crossing over the mast, leaving the final corridor or failing
  to demonstrate inward front ingress makes the mission result fail, even if
  a subsequent physical contact occurs. This is an observation and result
  condition, not a virtual wall or rigid-body constraint.
- CSV output includes the active crossrange correction and a clearance-reason
  mask. Bits 0–5 represent mast projection, frontal corridor, mast collision,
  arm opening, contact alignment and floor violations. When no reference has
  yet been accepted, the input columns expose the latest planning attempt.

The shared PowerShell evidence check is used by both the physical flight
matrix and the rendered-flight test. Missing approach fields fail validation.

## Validation

The Unreal 5.8.2 Development Editor build succeeds. All 17 model tests pass
with zero warnings. New independent checks exercise the approach geometry with
tower headings of 0, 45, 90 and 180 degrees, including a rejected mast overflight
at 500 m, and verify the crossrange curve's endpoint conditions and numerical
derivatives. The previously recorded rear-approach planning input is rejected;
a measured frontal input is accepted, including the existing fuel-budget checks.

All nine complete physical flights pass the approach and passive-support
contracts. `Offset` is the existing scenario identifier for 5% additional dry
mass; it is not a spatially displaced launch site. The cadences below are game
update rates; actual Chaos substeps remain smaller and cadence dependent.

| Scenario | Game rate | Minimum mast-front margin | Minimum corridor margin | Post-capture support drift |
|---|---:|---:|---:|---:|
| Nominal | 60 Hz | 12.145 m | 0.883 m | 3.82 cm |
| Crosswind | 60 Hz | 11.870 m | 0.376 m | 3.18 cm |
| +5% dry mass | 60 Hz | 12.277 m | 1.028 m | 3.92 cm |
| Nominal | 30 Hz | 12.147 m | 0.882 m | 4.56 cm |
| Crosswind | 30 Hz | 11.872 m | 0.377 m | 3.83 cm |
| +5% dry mass | 30 Hz | 12.278 m | 1.025 m | 4.68 cm |
| Nominal | 15 Hz | 12.150 m | 0.881 m | 5.70 cm |
| Crosswind | 15 Hz | 11.875 m | 0.379 m | 4.72 cm |
| +5% dry mass | 15 Hz | 12.257 m | 0.997 m | 5.82 cm |

Each flight uses one accepted terminal reference, records inward front ingress,
has zero structural contacts, and ends on both physical rails with engine
shutdown. Peak terminal tracking error is 2.03–2.73 m. Maximum absolute
propellant-balance residual is 5.92 × 10⁻⁸ kg. Fitting alignment error at secured
support is 0.293–0.452 m; the booster-base axis error is a different quantity
because the supported body retains a small lean.

Landing burns still last 69.4–81.1 seconds. This wave validates the approach
direction and stable physical tracking; it does not complete burn optimization
or establish real-flight fidelity.

Evidence in `Saved/Recovery`: `FrontVerified-Unit/index.json`,
`FrontVerified-matrix.json`, the nine `FrontVerified_*.json`/CSV reports,
`FrontVerified-metrics.json`, and `FrontVerified-source.json`.

The separate rendered Crosswind flight also passes at 15 game updates/s,
1,920 × 1,080 output and DLSS at 66.7% internal resolution. Across 7,274 frames,
including 1,312 above 80 km, the actual route matches the corresponding physical
flight: 11.875 m minimum mast-front margin, 0.379 m minimum corridor margin and
zero structural contacts. The `CAPTURE` and `SECURED` images were visually
inspected: the booster approaches the projecting arms and finishes between them
in front of the mast.

During 121 secured Chase frames, the maximum frame-to-frame camera-offset
change relative to the vehicle is 7.28 × 10⁻¹² cm and the angular step is zero.
The captured camera remains stable; this statistic does not claim that every
camera transition throughout the mission has been exhaustively tested.

Fresh rendered reports and 15 review files are preserved under
`Saved/Recovery/FrontApproachRendered`. Both the physical and rendered source
manifests were checked after execution: all 370 recorded files, including the
compiled DLL, retain their SHA256 hashes. User preferences were restored byte
for byte. Verification is recorded in `FrontApproach-source-verification.json`
and `FrontRendered-settings.json`.

## Limits

The planner remains a sampled point-mass prediction with estimated aerodynamic
loads and a finite set of candidate curves. It is not a six-degree-of-freedom
optimal-control solution. Live rigid-body audits and varied flight tests are
required in addition to candidate feasibility.

The wind lead is a calibrated allowance for powered braking, not a complete
forecast of future gusts or engine failures. Unreachable-target detection and
a physically guided diversion remain open programme items. Tower arms still
use the existing kinematic mechanism; load-limited articulated tower dynamics
are also unfinished. The full [150-item programme](../REALISM_PROGRAM.md)
remains in scope.

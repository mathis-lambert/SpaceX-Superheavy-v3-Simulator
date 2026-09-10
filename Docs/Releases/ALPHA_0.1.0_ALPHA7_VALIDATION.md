# Alpha.7: interactive recovery, emergency braking and continuous atmosphere

10 September 2026. Source and packaged validation are recorded separately.
The release manifest identifies the exact game source commit. Alpha.6 remains
available in its existing directory. Nothing is pushed to a remote repository.

## Changes

- Visible pointer, held-right-button camera control, contextual actuator cards,
  explicit disable/restore/timed outage and a clickable registered-engine diagram.
- Live flight computer with measured state, actuator controls, event history,
  flown trail, ballistic estimate, accepted terminal plan and offshore candidates.
- Health-aware fin allocation and residual engine/RCS torque control. Displaced
  entry footprints can request a bounded corrective burn. Infeasible returns
  select an estimated alternative or impact mitigation. Emergency braking reserves
  turning/valve time and prioritizes vertical deceleration over lateral targeting.
- Native spherical atmosphere, separate stars, three cloud altitude bands and
  selectable weather. The planar fog background no longer cuts the orbital view.
  Regional/aerial colour matching and distance blending reduce photographic seams.
- Adjacent-frame sparse-volume interpolation, smooth domain-edge density falloff,
  continuous procedural descending condensation and narrower delivered-force RCS
  envelopes. Landing exhaust is separate from surface-interaction steam.
- Limited gimbal translation with a physical RCS counter-torque budget makes the
  final body attitude upright without imposing a pose. Filtered steel detail and
  compact in-flight controls complete the presentation changes.

See [implementation and model limits](../INTERACTIVE_RECOVERY.md).

## Source validation

Evidence is retained in [0.1.0-alpha.7](0.1.0-alpha.7/).

- 31 physics model tests pass without warnings, including immutable timed-fault snapshots, held
  countdown expiry, residual torque, conserved gimbal forces, corrective-burn
  decisions, fuel-limited alternates and emergency braking.
- Six physical return replays: Nominal, Crosswind and Offset at 15/60 Hz game
  cadence with a fixed 120 Hz solver. All pass physical support, front ingress,
  gentle first contact, fuel accounting and eight-second settled support.
- Four fault scenarios at both cadences: repeated one-second RCS outages,
  a twelve-second RCS outage, a jammed fin and combined engine/fin/RCS faults.
  All recover the tower. A separate 120-second coast RCS outage also recovers.
- A 65-second RCS loss beginning during separation triggers abandonment of the
  tower objective. The emergency replay measures real engine braking and surface
  contact at 1.70–1.81 m/s, with a failed tower mission, at both 15 and 60 Hz.
  This endpoint is sea level; it does not classify terrain from a longitude cutoff.
- Eight contact fixtures cover empty/open arms, unloaded closing, centered and
  along-rail support, wrong heading, side impact and overload.
- Rendered controls (69 checks), presentation (12) and world (9) audits pass.
  Manual visual inspection covers the 14/16/80 km horizon and launch steam frames.
  Screenshot-loop frame rates are not used as performance measurements.

Nominal first contact: vertical speed -0.2383 m/s, body tilt 0.2476 degrees,
angular speed 0.2960 degrees/s; settled tilt 0.3302 degrees and drift 0.0295 m.
The three reference landing burns span approximately 29–36 seconds.

## Render performance and crash investigation

Windows 11, Ryzen 5 9600X, RTX 4070 SUPER, 32 GB RAM. Normal-window DX12,
2560 x 1440, DLSS Quality, hardware tracing off. The launch measurement uses
6,000 frames with the first 300 excluded. The same view/settings baseline is
alpha.6; these are local measurements, not hardware-independent guarantees.

| Scene | Mean FPS | Mean frame | p95 frame |
|---|---:|---:|---:|
| Home | 58.01 | 17.24 ms | 18.44 ms |
| Launch | 58.62 | 17.06 ms | 21.41 ms |

Launch GPU mean is 14.78 ms; heterogeneous volumes average 0.72 ms (p95 3.10 ms).
Alpha.6 launch averaged 57.59 FPS, so the overall rendering cost is comparable.
The 427 ms maximum frame remains a hitch; the average does not hide that limit.

One automated `-RenderOffscreen` run exhausted the D3D12 residency command-list
pool and opened Crash Reporter. It was closed without transmitting a report.
Subsequent normal-window visual audits and the 6,000-frame launch run completed
without that failure or material compilation errors. Offscreen rendering is not
qualified by this release; no engine pool-limit or graphics-driver fix is claimed.

Material generators now delete a snapshot of existing expressions. UE 5.8's
bulk-delete loop can otherwise skip nodes and leave duplicate cloud advanced
outputs; rebuilt materials compile correctly.

## Remaining limits

One failed engine and one jammed fin can coexist with a shared RCS manifold
fault. Independent simultaneous engine-bank/nozzle failures are not implemented.
Predictions are point-mass or terminal-polynomial estimates, not a six-degree-of-
freedom reachable set. The corrective-burn branch has a controller contract test;
the recoverable fault replays do not require it. Alternate selection is revocable,
not a guaranteed landing at the selected marker. Ocean contact is an endpoint,
not a buoyancy, breakup or water-impact simulation.

Steam is an interpolated finite fluid cache, not coupled real-time CFD. Image
detail, cloud shapes, local ground seams and distant scenery remain alpha quality.
Hardware beyond this development machine has not been qualified.

## Packaged executable

The first package passed the functional flight but failed the strict log check:
an editor GameFeatureData scan rule attempted to load the absent GameFeatures
runtime class. The rule is now editor-only, as in alpha.6. This failed candidate
is retained under Saved/Recovery/RejectedPackages and is not the delivered build.

The configuration-corrected alpha.7 passed startup without the ensure. Its control
audit then hit a wall-time assumption while a second user-launched instance shared
the GPU: the countdown was still advancing, rather than stuck or reset. This
candidate is superseded by alpha.8, which waits for measured ascent with a finite
timeout. The user's running alpha.7 directory is preserved. See
[alpha.8 validation](ALPHA_0.1.0_ALPHA8_VALIDATION.md) for the final package status.

# Dynamic landing ignition and front capture

This is the historical alpha.4 ignition and guidance comparison. Alpha.6 adds
dynamic tower suspension and a 0.25 m/s contact target; current measurements and
limits are in [the alpha.6 validation](Releases/ALPHA_0.1.0_ALPHA6_VALIDATION.md)
and [coast, volumes and tower](COAST_VOLUMES_TOWER.md).

Alpha.4 replaces the early landing trigger and long powered approach with a
predicted braking envelope. The reference is the pacing of the official
[Starship Flight 5 broadcast](https://www.youtube.com/watch?v=hI9HQfCAw64): coast,
decisive landing braking, alignment and capture. The video description was
available for research; no frame-derived timing or private telemetry is claimed.
The simulator retains its estimated V3 vehicle configuration, rather than
claiming to reproduce Flight 5's exact vehicle or guidance.

## Physical behaviour

- The ignition predictor integrates vertical braking with live vehicle mass,
  propellant, available central/ring engines, pressure-dependent specific impulse,
  gravity, drag and the current vertical thrust projection. It uses the same valve
  impulse integration as the physical engines, including their opening response
  and closing tail. A failed engine is excluded; fuel cannot be borrowed.
- The prediction is refreshed every 0.1 seconds below 10 km. Ignition occurs when
  the remaining height reaches the predicted stopping distance plus an 80 m
  reserve and one decision interval of descent. There is no fixed 4.5 km ignition
  threshold. The old serialized property `LandingIgnitionCeilingM` survives only
  as the return wind reference altitude, explicitly labelled in the editor.
- The high-thrust bank removes the main descent energy; the three central engines
  handle the final transfer. The guidance envelope allows up to 55 m/s² requested
  net braking and 20° requested tilt. Rated thrust, minimum throttle, 8° gimbal
  travel, gimbal rates, valve response and attitude actuators are unchanged.
- Lateral braking accounts for current closing speed and the time needed to stop
  each horizontal component. The terminal search retains continuous initial
  position, velocity and acceleration and checks thrust, fuel, turning rate and
  the tower's front corridor. Its wind forecast now follows the same height
  profile as the force model, including reduced wind near the ground.
- The polynomial is a guidance reference. It supplies actuator requests, never
  vehicle transforms or prescribed velocities. Chaos still integrates every
  engine, aerodynamic, gravity and contact load on the fixed physical clock.
- A verified upward fitting contact, safe heading/tilt/speed and usable rail
  footprint initiate engine shutdown. A longitudinal offset along a rail does not
  require thrust-driven recentering. Gravity transfers the weight to the second
  rail; capture requires sustained independent support from both rails.
  The arm opening retains clearance for the frame below the support surface
  during settling. Contact diagnostics use a consistent component transform so
  a delayed display frame cannot mix positions from two different samples.

The alpha.4 guide targeted 0.6 m/s descent at contact. Reported first-contact speed is the
incoming rigid-body sample preceding the first observed support impulse, rather
than the nearly-zero velocity after settling. Fitting-point impact velocity and
structural compliance remain separate future refinements.

## Measured change

These are full-flight solver results at 120 Hz physics / 60 Hz game cadence,
compared with the retained `FixedPhysicalMatrix` reference runs. They are simulated
values, not observations from the broadcast.

| Scenario | Previous thrust time | New thrust time | New ignition height | Ignition descent speed | First-contact body speed | Additional unpowered time |
|---|---:|---:|---:|---:|---:|---:|
| Nominal | 73.55 s | 30.86 s | 2,096 m | 396 m/s | 0.73 m/s | 5.34 s |
| Crosswind | 81.65 s | 31.58 s | 2,095 m | 396 m/s | 0.95 m/s | 5.35 s |
| Offset / increased dry mass | 69.52 s | 32.21 s | 2,259 m | 415 m/s | 0.85 m/s | 4.82 s |

All three maintain front ingress, zero structural collisions and both physical
rail supports with propulsion off. Time spent below 100 m above the catch with
total speed under 5 m/s, before first contact, is 4.5–5.4 s. This retains a short
alignment and contact interval without the previous prolonged slow approach.

## Verification and limits

`Tools/Tests/analyze_dynamic_return.py` gates shorter thrust time, additional
unpowered descent, soft incoming contact, a short slow interval, the front
corridor, physical support, thrust/gimbal bounds and the fuel ledger. The regular
flight harness also checks the physical clock, independent upper-stage dynamics,
separation momentum and eight seconds of unpowered support.

Model tests cover increased mass/speed, delayed valve response, failed central
engines, depleted fuel, inverted attitude and reachable horizontal transfer time.
Unpowered contact fixtures include centered support, a 3 m longitudinal rail
offset, wrong heading and a structural side strike. The latter two must not be
misreported as successful support.

The vertical predictor is an approximate reachability estimate with reserves,
not a proof of globally minimum ignition altitude. The terminal search is sampled
and receding-horizon, not a certified optimal controller. Atmospheric/aerodynamic
coefficients, dry mass, engine transients and structural-load limits are estimated.
Successful nominal, wind and mass-offset runs do not establish robustness to all
failures or numerical convergence of every possible return. Alpha.4 retained the
then-existing kinematic arm mechanism and passive contact model.

Release records under `Docs/Releases/0.1.0-alpha.4` identify the tested executable
and additional cadence/rendered checks. Site, sun, photographic controls and
assets from alpha.3 are retained.

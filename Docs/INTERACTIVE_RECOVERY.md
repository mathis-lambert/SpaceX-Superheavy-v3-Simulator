# Interactive recovery

## Viewer

The pointer stays visible. Hold the right mouse button to rotate an orbit or
free camera, then release it to return to pointing. Automatic cinematic orbit
is opt-in. Camera sensitivity, optics and saved photographic looks remain in
Settings. The toolbar opens Cameras, Computer and Weather and changes playback
speed. Escape closes an inspection panel first, then opens pause.

Left-click a visible engine nozzle, grid fin or RCS outlet to inspect it.
Selection alone never injects a failure. Disable, Restore and a one-simulated-
second outage are explicit actions. Occluded parts cannot be picked through the
hull. Computer > Systems provides an underside diagram based on the registered
33 engine mounts, including engines hidden from the current camera.

The experiment model currently supports one failed engine and one jammed fin
at a time, plus the shared RCS manifold. Selecting another fault in the same
family replaces it. RCS selection controls the manifold, not an independent
nozzle valve. Timed outages also expire during preflight countdown holds.

## Flight computer

- Flight: measured attitude/rates, ballistic miss, accepted terminal-plan
  tracking error, candidate rejection counts, predicted braking and rail loads.
- Systems: actuator selection, restoration, wind and attitude response.
- Events: timestamped phase and experiment records.

Grey shows the flown trail; cyan is the point-mass, engines-off impact estimate;
white is the accepted terminal plan. Offshore candidate rings include estimated
lateral delta-V and a reachability screen. These are different calculations,
not equally accurate predictions. No rejected candidate is drawn as an accepted
plan, and an unavailable tracking solution is shown explicitly.

## Physical recovery

The fixed 120 Hz solver still owns vehicle motion. Guidance commands bounded
engines, gimbals, grid fins and RCS; it never writes an active-flight pose.
Fin allocation redistributes residual torque around a jammed fin. Reaction jets
cover the torque not delivered by engines and fins, including during powered
flight. Their ambient-pressure efficiency is an estimated model; the reference
relation is [NASA's rocket thrust equation](https://www1.grc.nasa.gov/beginners-guide-to-aeronautics/rocket-thrust-equation/),
not published Super Heavy actuator calibration.

A displaced entry footprint can request a corrective burn when altitude,
dynamic pressure, alignment and landing reserve permit. Hysteresis avoids rapid
on/off commands. Stale plans are rejected when capability, tracking or expiry
invalidates them. An infeasible return can select an authored offshore candidate;
if none passes the screening calculation, the display says impact mitigation.
Neither a diversion nor an ocean contact counts as a successful tower capture.
Emergency braking includes attitude-acquisition and valve-opening time. Once
committed, it prioritizes vertical deceleration over a distant lateral target,
then switches to the core engine bank for the final descent. A stale offshore
estimate is revoked when the remaining terminal authority cannot close its miss.

Offshore screening is a point-mass estimate of time, lateral authority and fuel,
not a certified six-degree-of-freedom reachable set. It has no real-world hazard
map or full water-impact model. The controller cannot recover arbitrary faults,
insufficient fuel or a trajectory outside remaining actuator authority.

Near the rails, limited lateral gimbal translation allows the body to remain
upright. The allocation is capped by available RCS counter-torque and fuel.
Physical contact, compliant rails and their loads determine the settled pose;
there is no final pose snap or vehicle-to-tower weld.

## Atmosphere and propulsion

Ground, Earth and atmosphere share a spherical reference. Native Sky Atmosphere
renders scattering; a separate additive material supplies stars. The global
planar height-fog term has zero maximum opacity while its volumetric grid remains
available for local steam. Weather blends marine, middle and high cloud layers
and changes wind; Clear, Coastal haze, Broken clouds and Overcast are available.

Turbulent ground steam shares a 64-frame sparse volume cache, blends neighbouring
frames and fades density before each domain boundary. Source playback spans the
full four-second billow life. Procedural cryogenic condensation follows a narrow
downward flow with several turbulence scales and an extended dispersing tail.
Airborne landing exhaust begins while engines run; ground deluge steam remains
separate. These are bounded real-time visual approximations, not a coupled CFD
simulation. RCS effects follow delivered force with a much narrower jet envelope.

The Starship steel material filters weld and brushed detail by pixel footprint
and uses restrained roughness variation. Material generators delete a snapshot
of the old expression list to avoid skipped nodes in UE 5.8's bulk deletion.

## Reproduction

Build with `Tools/Runtime/build_simulator.ps1`. With Unreal closed, regenerate
the presentation assets using `Tools/Editor/build_interactive_presentation.py`
in a rendering-enabled Python commandlet. Run `test_physics_models.ps1`,
`test_physical_recovery.ps1`, `test_resilience.ps1`, `test_emergency_recovery.ps1`
and `test_contact_fixtures.ps1`.
Rendered controls, weather/altitude and vapor checks are separate from headless
flight success. Match output resolution, reconstruction, view and hardware when
comparing performance. Release-specific evidence belongs under Docs/Releases.

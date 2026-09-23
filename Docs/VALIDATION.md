# Validation — 9 September 2026

## Current wave: fixed physical clock and solver contact observations

[Fixed-clock evidence](Validation/FIXED_PHYSICS_CLOCK.md) records the 120 Hz
booster/Starship integration, atomic mechanical separation and support observation
directly from Chaos. Twenty model tests, three unpowered contact fixtures, nine
flight scenarios, 59/144 FPS runs and repeated 200 ms display stalls pass.
The 120/240/480 Hz runs expose unresolved whole-return timestep sensitivity;
requirement 72 remains open. The user has frozen new feature work for an initial
Windows alpha release.

## Previous wave: front approach through the tower opening

[Front approach evidence](Validation/FRONT_APPROACH.md) records the revised
boostback/entry target, terminal lateral alignment and continuous audit of the
actual recovery route. Seventeen unit checks and nine physical flights pass.
The rendered Crosswind flight passes with front ingress, two supported fittings,
zero structural contacts and stable Chase framing during secured support.
The approach audit is independent of whether the fittings eventually find the
rails; a path behind or over the mast cannot pass by subsequently making contact.

## Previous wave: terminal reference tracking

[Terminal guidance evidence](Validation/TERMINAL_REFERENCE_GUIDANCE.md) records
the development regression, reference tracking, load-transfer correction and
fresh physical checks. Fuel optimization and full tower dynamics remain open.

## Previous wave: flight guidance in Chaos

[Solver guidance evidence](Validation/SOLVER_GUIDANCE.md) records the migrated
navigation/predictor/phase loop, numerical decision snapshots, twelve unit checks
and nine physical flights. Nominal cadence sensitivity decreases, but fixed-rate
whole-flight control and terminal-descent calibration remain unfinished.

## Previous wave: booster inner dynamics in Chaos

[Solver dynamics evidence](Validation/SOLVER_DYNAMICS.md) documents the callback
integration, shared state, independent physics checks and remaining outer-loop
work. The full 150-item scope remains in [REALISM_PROGRAM.md](REALISM_PROGRAM.md).

## Previous wave: engine impulse and solver scheduling

[Propulsion and scheduling evidence](Validation/PROPULSION_AND_SCHEDULING.md)
records the value-only engine-bank extraction, eight passing unit tests and nine
passing physical flights. The actual Chaos/control cadence measurement exposes
the remaining frame dependence; this wave does not complete the fixed-step work.

## Previous wave: launch conditioning and full-mission cloud evaluation

See [ground and cloud validation](Validation/REALISM_GROUND_AND_CLOUDS.md) for the
current optical correction, physical launch-abort evidence and full-flight timing.
The older scene-presence checks below did not prove visible condensation; fresh
home/day/night image inspection is the stronger evidence. The 150-requirement
program tracks the remaining scope in [REALISM_PROGRAM.md](REALISM_PROGRAM.md).

## Previous wave: controls, menus, condensation and cleanup

Unreal 5.8.2 Development Editor build passes. Real input dispatch and Slate focus
checks pass before launch and during ascent: the legacy F1/F2/F3, number and R/X
keys preserve the flight and experiment state; I/L, Tab, J/K and pause work.
The interface audit also passes display timeout/rollback while paused and native
to DLSS transitions. Reports: `ControlsAudit/result.json`, `InterfaceAudit/result.json`.

The fresh asset audit compiles five Blueprints and resolves 153 dependencies.
Four site-detail materials have their instancing usage flags saved; fresh rendered
logs no longer report their fallback-material warning. Eleven legacy vehicle
Blueprint input events were removed. There are 267 unused package files outside
active Content, with SHA256 recovery copies and a dependency inventory in
`Saved/Recovery/UnusedContent`. Active Content is approximately 594 MiB.

A complete rendered Crosswind flight passes after the Blueprint/content cleanup:
6,356 frames, 97.563 km peak, both physical rail contacts and engine shutdown.
After capture, support drift is 2.94 mm over the measured eight seconds. Across
122 captured Chase frames, maximum camera offset step is numerical zero
(7.72e-12 cm), and maximum angular step is 0.0000054 degrees. No flight forces,
guidance gains, pose locks or capture constraints were added in this wave.
Reports: `ExperienceRenderedFlight.json`, `experience-flight-audit.json`.

Reviewed day/night images in `ControlsAudit` show continuous cold condensation
along the hull, wind deformation and local illumination. The two-volume optical
model is not CFD. Service traffic, three wind flags, emissive warning lamps and
two bounded headlight beams add decorative activity. The final headlight adjustment
was reviewed and benchmarked after the full-flight run. Ground/ascent vapor remains
the existing 128-volume system and still needs further fluid-shape refinement.

Ryzen 5 9600X / RTX 4070 SUPER, 2,560 x 1,440, Epic, solar time 17.9, hardware
Lumen off, VSync off, uncapped, no frame generation:

| Scene | Native TSR 100% | DLSS Quality 66.7% | Frame-time p95, native / DLSS |
|---|---:|---:|---:|
| Home | 39.65 FPS | 61.15 FPS | 26.389 / 17.560 ms |
| First 40 simulation seconds | 39.81 FPS | 65.64 FPS | 33.869 / 18.841 ms |

Each cell is one capture, excluding 300 warm-up frames (900 Home / 2,100 Launch
frames measured). Launch uses fixed 60 Hz and the default booster camera. The
benchmark now explicitly fixes time of day and hardware ray tracing. Earlier
measurements below inherited some scene preferences; these are not an identical
scene before/after comparison or evidence of a new universal speed gain.
Clouds remain the largest GPU cost: 11.873 ms native / 5.895 ms DLSS at launch.
Detailed heterogeneous condensation costs 0.366 ms at home with DLSS.
No 4K, full-flight frame-rate or hardware-ray-tracing speed claim is made.

Raw captures and summaries: `ControlCleanup{Home|Launch}{Native|DLSSQuality}1440`.
Reproduce with `Tools/Tests/measure_experience.ps1 -Prefix ControlCleanup -Hour 17.9 -HardwareRayTracing 0`.
Implementation and remaining priorities: [controls and cleanup](CONTROLS_AND_CLEANUP.md).

## Previous wave: world continuity and flight inspection

The world-continuity and flight-inspection wave builds with Unreal Engine 5.8.2. Combined evidence: `Saved/Recovery/world-validation-summary.json`, including source hashes. Earlier measurements from the retired strict-physics report remain available in Git history.

## Physics and Chase orbit

- Nine complete flights passed: Nominal, Crosswind and Offset at 60, 30 and 15 simulation updates per second. All reached physical rail capture and remained supported with engines off.
- Peak altitude: 97.271–97.564 km. Maximum post-contact support drift: 6.79 cm. Maximum capture alignment error: 15.87 cm. Maximum main-propellant balance residual: 2.98 × 10⁻⁸ kg.
- Centered, WrongHeading and SideImpact contact fixtures passed. Misalignment and lateral interference remain physical obstructions.
- Four unit tests passed with zero warnings/failures: fuel-limited impulse, engine moment allocation, split-stage mass/inertia conservation, and Chase contact stability. The camera test covers 60/30/15 Hz, alternating contact-velocity noise and bounded reacquisition.
- The rendered Crosswind flight in Chase orbit passed over 6,356 frames, including 1,321 above 80 km. Capture completed at approximately T+416 s.
- During 122 captured Chase frames, maximum change in camera offset relative to the booster was 8.91 × 10⁻¹² cm (numerical zero); maximum angular change was 0.00000342°. The camera no longer uses near-zero contact velocity as a new heading.
- Cached camera versus view-target position error was 0 cm. Independent Starship artwork remained aligned with its physical body, with six visible exhausts.

These establish regression and numerical consistency. Landing burns still span 65.6–96.3 seconds, and aero/engine/tank coefficients remain estimates. Tower arms retain their existing kinematic mechanical approximation; the booster is a free rigid body supported by contact. Successful capture does not validate agreement with a real flight.

Reports: `physical-matrix.json`, `Physical_*.json`, `WorldUnitTests/index.json`, `ContactCentered.json`, `ContactWrongHeading.json`, `ContactSideImpact.json`, `ExperienceRenderedFlight.json`, `experience-flight-audit.json`.

## Interaction, assets and visual review

The World Audit passed nine checks covering the live non-pausing laboratory, actual failed-engine thrust closure, fin-angle retention, experiment inputs, physical force samples, time acceleration, restored actuator availability and supported hardware Lumen/UI state. The interface audit passed pause/resume, physical state preservation during pause, display rollback, engine lighting, and native → DLSS → native transitions. Apply and Back remain visible in the reviewed graphics page.

Five Blueprints compile and 150 runtime dependencies resolve, including the star material. Sixteen local terrain meshes use Nanite; all 19 Earth actors retain their no-collision scenery policy. Terrain import reports 11.72 m local grid spacing and a 1,024 × 512 globe. Seven parked props use two Nanite meshes. Their final cab/wheel refinement was reviewed in Blender and imported through the shared scenery import policy.

The rendered flight observed 128 participating vapor volumes, three distributed plume lights, eight site spotlights and two playing audio sources. Volume extinction is connected, compiled and nonzero. This checks the rendering path; it does not certify fluid accuracy or AAA smoke quality.

Reviewed images: `WorldAudit/Home.png`, `Display.png`, `FlightLab.png`, `HardwareRayTracing.png`, `Horizon78km.png`, `Coast78km.png`, `Stars78km.png`; `Review/COAST.png`, `CAPTURE.png`, `SECURED.png`; and `service-props-source-review.png`. Paths are relative to `Saved/Recovery`, with WorldAudit names remaining under that folder.

Fresh rendered audit logs contain no failed-material compilation. Material-authoring logs contain some transient missing-input warnings during graph reconstruction; subsequent fresh-load and rendered checks are the acceptance evidence.

## Measured rendering performance

Ryzen 5 9600X / RTX 4070 SUPER, 2,560 × 1,440 output, Epic preset, VSync off, FPS uncapped, hardware Lumen disabled. Native TSR renders at 100%; DLSS Quality runs at 66.7%. Fog compensation remains enabled (8 native scene pixels versus 5 reconstructed scene pixels). Frame generation is absent.

| Scene | Native TSR | DLSS Quality | Frame-time p95, native / DLSS |
|---|---:|---:|---:|
| Home / Starbase | 41.94 FPS | 66.10 FPS | 25.172 / 16.125 ms |
| Launch / first 40 simulation seconds | 47.75 FPS | 81.58 FPS | 25.293 / 15.270 ms |

Each cell is one capture, excluding the first 300 frames: 900 measured home frames and 2,100 launch frames. Launch uses the same fixed 60 Hz sequence in both modes. The previous wave measured 29.67/48.16 FPS at home and 31.29/53.04 FPS at launch with the same protocol, but different world, sky and vapor implementations. This is not an identical-image comparison.

Launch GPU time averaged 19.526 ms native and 10.681 ms DLSS. Clouds accounted for 7.804/2.329 ms and fog 1.418/1.193 ms. The final service-prop cab/wheel refinement was imported after these captures. No 4K, peak-VRAM, full-flight FPS or ray-tracing performance claim is made.

Raw data: `WorldContinuityHomeNative1440.csv`, `WorldContinuityLaunchNative1440.csv`, `WorldContinuityHomeDLSSQuality1440.csv`, `WorldContinuityLaunchDLSSQuality1440.csv` and corresponding `.summary.json` files. Reproduce using `Tools/Tests/measure_experience.ps1 -Modes @(0,3) -Prefix WorldContinuity`, then `analyze_performance.py`. Saved preferences are restored after measurement.

## Remaining acceptance

The 100-item programme is still open: calibrated aero/sensors, slosh/thermodynamics, dynamic tower arms, efficient terminal guidance, surveyed multiple pads, driveable terrain collisions, replay/debrief, richer weather, finer vapor self-shadowing, coastal water and 4K/VRAM measurement. Continuous imagery removes prominent geometry-coverage rectangles in the reviewed views; this does not establish seamless appearance everywhere or remove every source-mosaic difference.

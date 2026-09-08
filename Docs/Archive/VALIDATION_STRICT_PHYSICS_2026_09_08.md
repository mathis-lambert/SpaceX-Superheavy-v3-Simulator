# Validation — 8 September 2026

The editor target builds successfully with Unreal Engine 5.8.2. Reports and screenshots below are local artifacts under `Saved/Recovery`.

## Current strict-physics wave

The new flight model passes nine complete flights (Nominal, Crosswind and Offset at 60, 30 and 15 Hz), three contact fixtures and three actuator/mass unit checks. The final headless flight matrix includes engine-group hysteresis. These results establish regression and numerical consistency, not agreement with proprietary flight data.

- Peak altitude across the nine runs: 97.271–97.564 km.
- Worst passive support movement during the eight-second catch check: 6.73 cm; final speed below 0.1 m/s in every case.
- Maximum fitting alignment error at capture: 15.71 cm.
- Every run registers 33 engines, preserves engine thrust-vector magnitude and respects the 8° gimbal limit.
- Maximum separation linear-momentum relative error: 1.90 × 10⁻⁸; angular-momentum relative error: 0.0876%.
- Maximum absolute main-propellant balance error, including ground supply and vents: 2.24 × 10⁻⁸ kg.
- The unit suite checks final-fuel impulse, moment allocation and split-stage mass/first-moment/inertia conservation: three passed, zero warnings/failures.
- Landing burns still span 67–97 seconds and peak dynamic pressure is approximately 158–160 kPa. These are unresolved calibration issues, not validated real-flight values.

The first rendered native flight after this work passed the camera, vapor, light, audio and capture checks. The final render checks additionally measure the independent Starship artwork against its physical body and require all six exhausts plus the instanced site detail. Current reports: `strict-flight-summary.json`, `physical-matrix.json`, `StrictUnitTests/index.json`, `experience-flight-audit.json`, `InterfaceAudit/result.json`.

Sixteen replacement Earth textures were imported from verified 4,000 × 4,000 USGS sources, with original Unreal assets preserved and hashed. The asset audit resolves all runtime dependencies and compiles the five Blueprints. New site geometry is 3,481 decorative instances in eight batches, without collision. Python and PowerShell authoring tools parse successfully.

## Current rendering comparison

Final `StrictRelease` captures on Ryzen 5 9600X / RTX 4070 SUPER, 2560 × 1440 output, Epic settings, VSync off and uncapped FPS. Launch follows the same fixed 60 Hz first-40-second sequence in each mode. The first 300 frames are excluded: 900 home frames and 2,100 launch frames remain. Each cell is one capture, not a statistical guarantee.

| Scene | Native TSR, 100% | DLSS Quality, 66.7% | Frame-time p95, native → DLSS |
|---|---:|---:|---:|
| Home / Starbase | 29.67 FPS | 48.16 FPS | 35.209 → 22.242 ms |
| Launch | 31.29 FPS | 53.04 FPS | 42.594 → 23.219 ms |

The final DLSS comparison compensates fog grid size (5 scene pixels versus the native 8) to retain approximately the native output footprint for vapor detail. Frame generation and ray reconstruction are not enabled. The earlier preliminary DLSS launch result of 62.34 FPS used a coarser output fog grid and predates the final HUD/render validation; it is not the delivery figure.

Final launch GPU times: 25.783 ms native versus 13.927 ms DLSS; clouds 13.609 versus 6.025 ms; fog 1.796 versus 1.539 ms. Game-thread time remains about 7.5 ms. A single in-run GPU snapshot reported 6,198 MiB used of 12,282 MiB, 68°C and 97% GPU utilization; this is not a peak VRAM capture or a process-isolated allocation measurement.

Native performance is below the earlier 40.42 FPS baseline. The additions and current render state have not achieved the desired native headroom. Cloud rendering and frame pacing remain priorities; no claim of cost-free visual improvement is made. Native/DLSS screenshots retain small-structure and menu readability, but this limited inspection does not certify every moving silhouette or translucent effect. High-altitude coverage/material boundaries and smooth isolated vapor billows remain visible.

The final DLSS Crosswind flight passed over 6,377 sampled frames, including 1,321 above 80 km. Cached camera position mismatch and independent Starship render/body position mismatch were both 0 cm. Maximum camera angular mismatch was 0.0000054°. The audit observed six Starship exhausts, 128 vapor volumes, 3,481 industrial instances, three plume lights, eight site spotlights and two playing audio sources. It also verified actual DLSS execution at 66.7% and the compensated fog grid.

The interface audit verifies native → DLSS → native transitions, hardware fallback, physical state preservation, pause/display rollback and engine light state. `InterfaceAudit/ReconstructionNative.png` and `ReconstructionDLSS.png` show the same settings page in the two modes. `Review` contains final flight phase, cloud-crossing and Earth-horizon captures; `Overhaul` contains day/night views from the initial native visual pass.

Raw data: `StrictReleaseHomeNative1440.csv`, `StrictReleaseLaunchNative1440.csv`, `StrictReleaseHomeDLSSQuality1440.csv`, `StrictReleaseLaunchDLSSQuality1440.csv`, with matching `.summary.json` files. Reproduce using `Tools/Tests/measure_experience.ps1 -Modes @(0,3) -Prefix StrictRelease`, then `analyze_performance.py`. Saved display preferences are restored after the benchmark. These are 1440p measurements, not 4K results.

## Earlier rendering baseline

RTX 4070 SUPER, native 2560 × 1440 output, 100% scene resolution, Epic settings, VSync disabled, uncapped frame rate. The same startup commands were used for the before/after captures. Each result is one capture, excluding the first 300 frames and CSV metadata. These are not 4K measurements or guarantees for every camera.

| Scene | Before | Final | Frame time before → final |
|---|---:|---:|---:|
| Home / Starbase | 26.08 FPS | 40.70 FPS | 38.350 → 24.572 ms |
| Launch / first 40 simulation seconds | 19.77 FPS | 40.42 FPS | 50.584 → 24.741 ms |

The launch capture uses a fixed 60 Hz simulation step and 2,100 measured rendering frames. The home capture uses 900 measured frames. Launch frame-time p95 improved from 67.424 to 32.733 ms. Cloud GPU time fell from 34.636 to 11.957 ms during launch. Final volumetric fog cost averaged 1.565 ms.

Full-resolution cloud tracing remains enabled. The principal optimization replaces secondary cloud shadow rays with cached cloud shadow maps; this changes the shadow approximation. Propulsion now uses fewer overlapping lights and 96 volumetric primitives instead of 1,280 translucent puff components. The final measurements include visible, illuminated vapor, site lights and audio.

Historical raw captures: `ExperienceBaseline/HomeNative1440.csv`, `ExperienceBaseline/LaunchNative1440.csv`, `FinalHomeNative1440.csv`, `FinalLaunchNative1440.csv`; each has a `.summary.json`. The benchmark script now accepts reconstruction modes and a capture prefix, and restores display preferences afterward. These earlier figures predate the current strict-physics wave and must not be presented as current measurements.

## Earlier physical and interaction baseline

- Nine physical flights passed: Nominal, Crosswind and Offset at 60, 30 and 15 simulation steps per second. Both fittings contacted the rails, engines shut down, and support remained stable during eight seconds without thrust. Maximum post-contact drift across these runs was approximately 1.9 cm.
- Centered, WrongHeading and SideImpact contact fixtures passed. Incorrect alignment and lateral impact remain physical obstructions.
- The final rendered Crosswind flight reached 97,114 m and 1,590 m/s, then completed capture. Post-contact drift was 1.14 mm. All 6,334 rendered frames were sampled, including 1,325 above 80 km. Maximum cached camera position error was 0 cm; angular error was 0.0000054 degrees.
- Direct mouse-axis events rotated both an orbit camera and the free camera without the right mouse button. Pause, nested settings, lighting while paused, playback control and display rollback passed. Fourteen camera modes were exercised.
- The runtime audit observed 85 active vapor volumes, three distributed plume lights, eight site spotlights and two playing audio sources. It checks that the vapor material compiled successfully and has nonzero volume extinction. The asset audit checks the actual RGB extinction connection.
- All 138 runtime dependencies resolve. Five Blueprints compile. The seven original clean vehicle meshes use Nanite and dedicated primitive collision policy. Twelve active vehicle/site artwork meshes were optimized in total. The old overlapping pad slabs and package redirectors are gone.
- Python authoring/test files parse successfully; PowerShell scripts parse successfully. Existing staged work was preserved.

Reports: `physical-matrix.json`, `ContactCentered.json`, `ContactWrongHeading.json`, `ContactSideImpact.json`, `ExperienceRenderedFlight.json`, `experience-flight-audit.json`, `experience-asset-audit.json`, `Overhaul/result.json`, `InterfaceAudit/result.json`, `EarthAudit/result.json`.

Screenshots: `Overhaul` contains menu/day/night views; `Review` contains flight phases and the Earth horizon. The final daytime flight screenshots include the last vapor material adjustment. Vapor is a procedural density model with a finite fog grid; distant trail detail still uses Niagara sprites. The audio playback path was checked in-engine, but no subjective listening assessment is claimed.

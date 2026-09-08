# Ground systems and cloud evaluation — 8 September 2026

Checkpoint `1d7e3c8` contains the corrected condensation material, launch audits
and phase-aware performance recording. This checkpoint extracts screenshot
recording from the flight director into the opt-in diagnostics component. Its
visual-review manifest records the actual camera, physical body, solar time,
resolution and render settings for each image. Screenshots do not run during
timing captures, since saving them would contaminate the measurement.

## Ground sequence and condensation

The Unreal 5.8.2 editor build succeeds. Four independent performance-analyzer
fixtures pass, including missing engine counters and known latency tails.
`Saved/Recovery/GroundAbortAudit/result.json` passes 12 ground checks: the
conditioned vehicle survives launch selection, a cold countdown can hold and
resume, water precedes ignition, engine thrust builds on the mount, and a real
post-ignition engine fault shuts all engine valves while retaining the mount
and cooling water. The report explicitly requires separate visual inspection.

Reviewed `GroundAudit/Home.png`, `ControlsAudit/VaporDay.png`,
`ControlsAudit/VaporNight.png` and `GroundAbortAudit/SafeShutdown.png` show
continuous condensation, scene lighting and safe shutdown without flames.
The old condensation billow fallback is removed. Isotropic 50 cm voxels and
conversion of inverse-metre extinction to local voxel units correct optical
depth. This is an estimated participating-medium flow, not a fluid solver.
Ground deluge and ascent trails still use the existing fog-grid volumes.

The countdown is an explicitly estimated final-minute rehearsal. Electrical
power transfer is currently a command label, not an electrical-system model;
tank thermodynamics and moving mechanical launch clamps remain open.

## Full mission timing

Ryzen 5 9600X / RTX 4070 SUPER; 2560 × 1440; DLSS Quality; Epic;
solar hour 17.9; hardware Lumen disabled; VSync disabled; no frame generation.
Both runs use the same nominal physical trajectory with a fixed 30 Hz simulation
update. This controls trajectory repeatability, and is not proof of fixed-step
guidance or numerical convergence. Wall-clock frame times are measured separately.

The production mode-3 run terminates its CSV after the mission result is written,
including eight seconds of physical support with engines off. It contains 13,421
measured frames after 300 startup frames. Maximum observed local RHI memory is
4,252 MiB, distinct from the texture streaming budget and system-memory counters.
No assertion about 4K residency or other cameras is implied.

| Phase | Frames in each run | FPS mode 3 | FPS mode 1 | Frame p99 ms, 3 / 1 | Cloud GPU mean ms, 3 / 1 |
|---|---:|---:|---:|---:|---:|
| Countdown, excluding startup | 1,499 | 75.13 | 98.54 | 15.08 / 12.77 | 4.22 / 1.12 |
| Ascent | 3,916 | 60.57 | 91.51 | 22.19 / 13.00 | 7.93 / 2.20 |
| Separation | 487 | 57.76 | 97.07 | 19.17 / 11.75 | 9.75 / 2.82 |
| Boostback | 1,100 | 55.72 | 93.83 | 19.53 / 11.83 | 10.09 / 2.95 |
| Coast | 2,388 | 58.13 | 100.04 | 18.72 / 11.15 | 10.02 / 2.84 |
| Entry | 1,664 | 61.36 | 95.36 | 19.79 / 13.33 | 8.35 / 2.47 |
| Landing burn | 1,142 | 96.38 | 100.30 | 15.36 / 12.03 | 0.85 / 0.31 |
| Capture | 984 | 101.91 | 101.09 | 11.81 / 12.11 | 0.07 / 0.03 |

The earlier candidate run includes a longer post-capture idle period. It is
excluded from this phase comparison; comparing the two whole-file FPS values
would use unequal windows. GPU pass durations may overlap and must not be
summed as an additive budget. PSO counters reported as -1 are unavailable.

Raw evidence under `Saved/Recovery`:

- `RealismBaselineFlightDLSSQuality1440Cloud3Hz30.{csv,summary.json,capture.json,log}`
- `RealismFlightFlightDLSSQuality1440Cloud1Hz30.{csv,summary.json,capture.json,log}`
- `RealismProbeHomeDLSSQuality1440Cloud{3|1}Hz60.*` — matched home comparison;
  58.72 / 82.36 FPS, cloud GPU mean 6.853 / 2.110 ms.

## Visual acceptance and reproduction

The installed engine's `VolumetricRenderTarget.cpp` identifies mode 3 as
full-resolution tracing with support for applying clouds to translucency. Mode 1
uses reconstructed tracing. [Epic's cloud guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine)
describes the opaque-intersection support of mode 1. Translucent rocket plumes,
fast cloud crossings and the orbital horizon therefore need paired review.

```powershell
./Tools/Tests/review_clouds.ps1 -Prefix RealismCloud -CloudModes 3,1 -Height 1440
./Tools/Tests/measure_experience.ps1 -Modes 3 -Scenes Flight -Heights 1440 -CloudModes 3,1 -Prefix RealismFlight -SimulationHz 30
python Tools/Tests/compare_visual_reviews.py Saved/Recovery/Review/RealismCloud1440Mode3 Saved/Recovery/Review/RealismCloud1440Mode1 --output Saved/Recovery/RealismCloud-comparison.json
```

Use the configured Python runtime if `python` is not on PATH. Capture wrappers
preserve user preferences and record a SHA256 source/content/config/module
inventory. The comparator refuses mismatched physical trajectories or views;
its pixel differences only identify images for inspection, not visual acceptance.

The completed mode-3/mode-1 flights produced 84 matching physical views each.
Sampled mode-1 cloud crossings show holes and ragged distant edges, particularly
`Cloud_042_05.png`. Mode 3 therefore remains the production setting.

## Inactive storm shader checkpoint

The cloud material now uses a static `EnableStormClouds` feature and one shared
storm-strength parameter in place of six duplicates. Clear-weather instances
compile out the inactive storm calculation. Cloud tracing resolution, sample
counts, scattering and lighting settings remain unchanged. The shared authoring
helper reproduces the material change during both targeted and full rebuilds.

`StormPrunedVisual1440Mode3` completed the nominal physical mission and produced
84 views matching the baseline geometry. Sampled crossing, horizon and entry
views retain their cloud silhouettes. The startup countdown has the largest
image difference (mean absolute RGB difference 1.92/255); paired inspection
shows a lighting difference on the ground. Its cause has not been isolated.
The sampled cloud views support retaining the optimization in mode 3. This is
not a claim of identical startup rendering or validation of every weather preset.

The warmed full-flight capture
`StormPrunedFlightFlightDLSSQuality1440Cloud3Hz30` completed with the cloud GPU
counter present. One CPU image-comparison job overlapped its initial countdown;
that phase is excluded from performance claims. The following later phases have
the same frame counts and settings as the production baseline above:

| Phase | FPS before / after | Frame p99 ms, before / after | Cloud GPU mean ms, before / after |
|---|---:|---:|---:|
| Ascent | 60.57 / 67.89 | 22.19 / 17.59 | 7.93 / 6.39 |
| Coast | 58.13 / 59.92 | 18.72 / 18.09 | 10.02 / 9.48 |
| Entry | 61.36 / 64.47 | 19.79 / 17.81 | 8.35 / 7.63 |

These are single-run observations, not a statistical guarantee. An earlier
`StormPrunedHome` capture lacked the cloud GPU counter and is excluded; the
measurement wrapper now rejects that case. Six Python validation fixtures pass,
the three PowerShell scripts parse, and the Unreal editor build succeeded.
The fresh `propulsion-ground-abort.log` run passes 12 checks. Its reviewed
`GroundAbortAudit/SafeShutdown.png` shows `T-00:00:01`, zero engines/thrust,
no flames, no highlighted flight phase and no landing-axis strip. Raw
screenshots, CSVs and build evidence remain local
under ignored `Saved/Recovery`; this document records their checkpoint status.

The planet's low-detail coastline, repetitive cloud layout, large-scale water,
metal texture repetition and deluge fluid motion remain separate open work.

# Alpha.6: coastal detail, dense vapor and softer physical capture

Build `0.1.0-alpha.6`, 9 September 2026. Packaged game source:
`646ce823c1d08094637a2fe0644aa012a3ff1a2c`.
The release evidence commit and tag add documentation only. Alpha.5 remains
available separately. No repository changes were pushed.

Open `Releases/Starbase-0.1.0-alpha.6/Windows/SuperHeavySim.exe`; keep the entire
Windows directory together. The portable ZIP includes the game, instructions,
source attribution and a file manifest. Unreal Editor is not required.

## Delivered

- Sixteen coastal meshes use available USGS LiDAR relief, with finer regional
  imagery bridging local photographs and the wider Earth. Shared shoreline
  masks join the water, sand and terrain shading across their different scales.
- Water retains its moving swells while replacing the crossed repeating normal
  texture with a bounded spectrum of twelve directional waves. Unresolved fine
  detail fades with pixel footprint. Vertex displacement no longer samples four
  orbital imagery layers unnecessarily.
- Original Blender flow caches supply eight shared, lit turbulent volumes.
  Larger, denser ground billows and wider turbulent volumes increase launch and
  landing coverage within the existing pools. Visible billows are not overwritten
  to make room for new ones. Frozen cache frames avoid redundant proxy updates.
- Two torque-driven physical tower hinges and two suspended rails transmit the
  actual fitting loads. Springs, dampers, travel limits and overload breakage
  remain active. Rail spacing now accounts for the fitting's position along the
  angled arms; the guidance requests 0.25 m/s vertical contact and reduces lateral
  oscillation. There is no catch attachment or imposed booster pose.
- RCS artwork follows delivered nozzle forces with a compressed optical response
  and short decay. Weak corrections are visible without changing their physical
  force, valve response or fuel consumption.

See [implementation and source coverage](../COAST_VOLUMES_TOWER.md).

## Exact executable validation

Windows 11, Ryzen 5 9600X, RTX 4070 SUPER, 32 GB RAM. The functional test uses
isolated user settings and 1920 × 1080 output with DLSS Quality. A complete rendered
Crosswind mission runs at a fixed 15 Hz game cadence and 120 Hz physics; these
simulation rates are separate from measured wall-clock rendering performance.

| Check | Result |
|---|---:|
| Startup resources | 34 / 34 |
| Pending shaders / PSOs / textures at reveal | 0 / 0 / 0 |
| Startup ready with local caches | 5.940 s from engine start |
| Controls and menu checks | 69 passed |
| Rendered Crosswind mission | Passed, 6,680 frames |
| Landing thrust duration | 32.042 s |
| First contact total / vertical speed | 0.515 / −0.234 m/s |
| First contact tilt | 0.700° |
| Structural contacts | 0 |
| Front ingress, both supports, engines off | Passed |
| Broken rails / hinges in the return | 0 / 0 |
| Passive support drift over eight seconds | 0.132 m |
| Chase camera continuity at contact | Passed over 121 frames |
| Peak active turbulent instances | 8 / 8 budget |
| Master mix | 48 kHz stereo, non-silent, zero clipped samples |

Loading, home, ascent, weak RCS, contact and secured captures were inspected.
The checked packaged logs contain no missing game asset, failed material, fatal
error or handled ensure. The screenshot-instrumented first-launch audit reports
14.73 ms P50 and 21.26 ms P95, with a 291.8 ms maximum; screenshot readbacks cause
stalls, so this is not a hitch-free gameplay claim.

Source validation passed 25 model tests, eight mechanical fixtures, six complete
returns at 15/60 Hz, nine world/interaction checks and the asset dependency audit.
Across those six returns, vertical contact speed is 0.234–0.256 m/s, tilt is
0.623–0.972°, total contact speed is 0.331–0.625 m/s, and landing thrust lasts
30.45–32.19 s. The low, slow portion lasts 4.27–5.58 s. Angular motion and passive
settling remain physical; this is not a perfectly motionless contact.

Wrong-heading, side-impact and overload fixtures validate their expected failure
behavior. All 24 shared terrain boundaries were checked after FBX round-trip,
with a maximum mismatch of 0.40 mm. Current flight evidence is in
`Docs/Validation/FinalPolish`; geographic and volume-contribution evidence is in
`Docs/Validation/CoastVolumesTower`.

## Performance comparison

The paired packaged captures use 2560 × 1440 output, DLSS Quality, cloud mode 3,
software Lumen and solar hour 17.9. Runs are sequential with isolated preferences.
Home records 1,200 frames; Launch records 6,000 including countdown and ascent.
The first 300 frames are excluded. The settings retain a 60 FPS limit, so average
FPS is not uncapped GPU throughput. GPU pass timings may overlap.

| Scene / statistic | Alpha.5 | Alpha.6 |
|---|---:|---:|
| Launch mean GPU time | 14.445 ms | 14.675 ms |
| Launch FPS from mean frame time | 58.35 | 57.59 |
| Launch frame P95 | 20.345 ms | 20.650 ms |
| Ascent-only mean GPU time | 15.533 ms | 15.962 ms |
| Home mean GPU time, two runs | 13.663–15.345 ms | 13.670–15.391 ms |

The paired launch run costs 0.230 ms more GPU time (1.6%) with the new terrain,
water and denser volumes. Its heterogeneous-volume pass rises from 0.452 to
0.623 ms averaged over countdown and ascent. These measurements do not establish
a general FPS improvement. The repeated Home runs reverse their ordering, with
variation concentrated in cloud/shadow passes; their overlapping ranges do not
support a reliable winner. No rendering quality setting was reduced for the
comparison. Phase summaries and original CSV hashes are recorded in
`0.1.0-alpha.6/performance-comparison.json` and its adjacent reports.

## Scope limits

USGS LiDAR covers 50.58% of the 12 km square; missing/offshore areas retain the
coarser elevation fallback. Regional output spacing does not exceed the detail
of its satellite source. Imagery dates differ from the authored site. Water is
an optical wave model, not surveyed bathymetry or hydrodynamics.

Sparse flow is simulated offline and replayed with advection and expansion.
It does not provide live exhaust CFD or fluid collisions with every site object.
Tower hardware parameters are engineering estimates; the carriage is an ideal
fixed bearing. These tests validate the implementation, not real tower loads or
whole-flight numerical convergence. Other GPUs have not been qualified.

## Reproduce

```powershell
./Tools/Tests/test_physics_models.ps1 -Prefix FinalPolish
./Tools/Tests/test_contact_fixtures.ps1 -Prefix FinalPolish
./Tools/Tests/test_physical_recovery.ps1 -Cadences 60,15 -Prefix FinalPolish
./Tools/Tests/test_experience.ps1 -SkipMatrix -ChaseReview -DetailReview -Reconstruction 3 -MenuAudits @('RecoveryWorldAudit')
./Tools/Tests/test_packaged_alpha.ps1 -Version 0.1.0-alpha.6
./Tools/Tests/measure_experience.ps1 -Executable Releases/Starbase-0.1.0-alpha.6/Windows/SuperHeavySim.exe -Prefix Alpha6Final -Modes 3 -Scenes Home,Launch -Heights 1440 -SimulationHz 60
```

Packaging requires clean committed sources and an unused release directory.
Do not overwrite the verified release to reproduce a new build.

## Archive

`Releases/Starbase-0.1.0-alpha.6.zip`: **1,388,981,802 bytes**, 57 files,
all manifest hashes checked and ZIP CRC verified.

SHA-256: `46b05ff93987b83860c401dd5a9d282d61cf13a513a2b0802c902cdb717b25a6`.

Build manifest, archive metadata, packaged validation, source hashes, image hashes
and timing summaries are versioned in `Docs/Releases/0.1.0-alpha.6`. Full original
captures and logs remain in the local `Saved/Recovery` directories named by those
reports. The packaged `FLIGHT.md` records the earlier alpha.4 ignition change;
`COAST-VOLUMES-TOWER.md` describes this version's revised contact and mechanics.

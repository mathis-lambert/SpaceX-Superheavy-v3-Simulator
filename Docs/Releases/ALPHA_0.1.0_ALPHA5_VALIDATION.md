# Alpha.5: propulsion presence and prepared startup

Build: **0.1.0-alpha.5**, 9 September 2026. The package manifest records the
exact source commit. Release validation and archive hashes are kept in
`Docs/Releases/0.1.0-alpha.5`. The previous alpha.4 remains intact.

Game source commit: `e364e009c19b39b8330e1f8179723544b95de2c1`.
The release tag additionally contains verification documents; game source and
content were not changed after packaging.

Launch `Releases/Starbase-0.1.0-alpha.5/Windows/SuperHeavySim.exe`, or extract the
complete portable ZIP. The Unreal Editor is not required. Keep the entire
packaged Windows folder together.

The portable archive is 1,205,340,241 bytes (about 1.21 GB). All 53 files match
the release manifest, and ZIP CRC verification passed. The archive, manifest
and actual game executable hashes are recorded in `0.1.0-alpha.5/artifacts.json`.

## Delivered behavior

Each engine's flame, lighting and vapor production follow delivered thrust.
Individual flame materials add variation without moving the physical vehicle;
short luminous decay and continuing billow advection soften shutdown. Plume
lighting is reduced from the previous overexposed illumination.

Seven bounded loop voices combine launch roar, original rumble, crackle,
coastal wind, cryogenic flow, deluge and tower drive. Distance delays source
history, attenuates volume and filters high frequencies. Release and fitting
contact transients follow physical mechanism events. Camera vibration uses
received acoustic power, with direct structural excitation in the mounted view.
The capture camera remains stable after engine shutdown.

An early Slate screen covers initialization. A matching viewport screen waits
for the 32 registered resources, constructed presentation components, known
pipeline compilation and texture streaming before fading into Starbase. Controls
and automated launches wait for that handoff. Loading counters reflect actual
work rather than elapsed-time percentages.

See [implementation and sources](../PROPULSION_AND_STARTUP.md) for the authored
sound generator, NASA source attribution and resource preparation details.

## Exact executable validation

Windows, Ryzen 5 9600X, RTX 4070 SUPER; isolated user preferences, 1920 x 1080
output with NVIDIA reconstruction at 66.7% internal resolution. The rendered
Crosswind mission uses 15 Hz fixed game cadence and 120 Hz physics. Timing
measurements are wall-clock observations, not that configured simulation cadence.

| Check | Result |
|---|---:|
| Startup resource registry | 32 / 32 |
| Pending shaders, PSOs, textures at scene reveal | 0 / 0 / 0 |
| Startup ready, existing local caches | 4.245 s from engine start |
| World preparation within that startup | 0.897 s |
| Packaged controls/menu checks | 69 passed |
| Full rendered mission | Passed, 6,650 frames |
| Physical landing thrust duration | 30.375 s |
| Incoming speed at first rail support | 0.998 m/s |
| Structural contacts | 0 |
| Front ingress / both supports / engines off | Passed |
| Passive support drift over eight seconds | 0.139 m |
| Chase capture continuity | Passed over 121 frames |
| Mechanical sounds triggered | Mount release + two rail contacts |
| Maximum simultaneously playing audio sources | 9 |
| Recorded master mix | 48 kHz stereo, 6.592 s, non-silent |
| Recorded peak / RMS / clipped PCM samples | 0.07623 / 0.02561 / 0 |
| First-launch frame P50 / P95 | 13.51 / 16.96 ms |
| First-launch maximum, including screenshot stalls | 265.42 ms |

The startup overlay, home, countdown vapor, ascent, capture and secured images
were visually inspected. No missing content, fatal error or handled ensure was
found in the checked packaged logs. This does not claim an auditory review or
zero shader permutations first encountered later in flight.

Source-side validation additionally passed 25 model tests and six complete
physical flights: Nominal, Crosswind and mass-offset cases at 15/60 Hz game
cadence. Each preserved the short burn, soft contact and zero structural contact
gates. Raw evidence is in `Docs/Validation/PropulsionStartup`.

## Limits

Sound is designed from a NASA Shuttle launch bed and original synthesized layers;
it is not a measured Raptor recording. Propagation uses straight rays, a bounded
Doppler approximation and an altitude fade, without shock or refraction solving.
Recorded mix validation checks for a real signal and clipping; it does not
replace a listening review. Smoke remains an optical/advection approximation.

Startup prepares known resources and waits for active work to drain. This does
not guarantee that every driver cache, quality change or future effect permutation
is hitch-free. Frame measurements from automated capture include screenshot
readbacks and are not a 4K gameplay benchmark. The alpha.4 physical flight model
and its existing fidelity limits remain in effect.

## Reproduce

```powershell
./Tools/Runtime/build_simulator.ps1
./Tools/Tests/test_startup.ps1
./Tools/Tests/test_experience.ps1 -SkipMatrix -SkipAssets -MenuAudits @() -ChaseReview -Reconstruction 3
./Tools/Runtime/package_alpha.ps1 -Version 0.1.0-alpha.5
./Tools/Tests/test_packaged_alpha.ps1 -Version 0.1.0-alpha.5
python Tools/Runtime/archive_alpha.py Releases/Starbase-0.1.0-alpha.5 --validation Docs/Releases/0.1.0-alpha.5/validation.json
```

Packaging requires committed source and a new output directory. Executables and
archives remain outside Git. No remote push was made.

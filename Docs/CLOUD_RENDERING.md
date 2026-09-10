# Cloud renderer

## Diagnosis and references

The previous weather material sampled three full-detail fluid-noise volumes at
every occupied ray step. Cinematic full-resolution cloud tracing (mode 3) and
high-quality aerial perspective compounded that cost. A 2,560 x 1,440 DLSS Quality
stationary probe measured 37.95 ms in the cloud GPU pass at 94 km; the full frame
took 49.08 ms. Launch-only measurements were not representative of orbital views.

The circular globe cutoff came from limiting trace start distance to approximately
camera altitude plus 2,000 km. A planet's visible tangent is farther away. The new
limit is sqrt(h * (2R + h)) plus a shell margin. The cloud material remains one
spherical field across all camera modes; there is no switch to a replacement dome.

Primary references consulted:

- [Epic: volumetric clouds](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine): reactive temporal rendering, conservative density, bounded sample counts and atmospheric lighting.
- [Guerrilla: Horizon cloudscapes](https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn): cloud type/shape/lighting separation and a small GPU budget.
- [Guerrilla: Nubis Evolved](https://www.guerrilla-games.com/read/nubis-evolved): fly-through rendering is a separate performance problem from distant skies. This project does not implement the complete Nubis renderer.
- [Epic UE-388559](https://issues.unrealengine.com/issue/UE-388559): documented distant-geometry artifacts in bilateral upsampling. Use depth-aware jittered upsampling (3), and inspect moving silhouettes rather than assuming the default bilateral path is artifact-free.

The local UE 5.8 source confirms mode 3 traces at full resolution, mode 0 traces
at quarter resolution and reconstructs at half resolution, and mode 2 cannot
intersect opaque objects correctly. Mode 0 is the final default, paired with
up to 192 depth samples near cloud layers, approximately 58 from 90 km, and
eight shadow samples over 4 km. The depth budget changes continuously from 12 km.
Mode 1 was also measured; dense native-resolution cloud-top views cost too much.
The chosen policy spends more samples on depth structure rather than tracing
every screen pixel. Opaque silhouettes still participate in composition. Image
reconstruction and scene render resolution are unchanged by the cloud policy.

## Authoring contract

`Tools/Art/build_cloud_noise.py` creates seeded, periodic, quintic-interpolated
coherent noise combined with Worley cellular fields. These are project-authored
textures, not a downloaded fluid cache. `ArtSource/Weather/manifest.json` records
their contract. The material builder imports the volume atlas and spherical
coverage/type texture into `Content/Starbase/Textures/Weather`.

`Tools/Editor/build_layered_weather.py` is the single material authoring entry.
It creates marine, middle and high layers. Conservative density rejects empty
altitude bands and clear-weather regions before volume evaluation. Distant rays skip
the detail fetch and increase the shape mip continuously; fine erosion remains
nearby. Sun transmittance is evaluated at each sample, so the planet is not lit
using Starbase's sunset color everywhere. Wind advects the field in metres/second.
An independently rotated low-frequency field warps the small tile and modulates
cloud banks. The clear, broken and overcast settings vary occupied volume, rather
than using different camera-dependent sky assets.

The directional light's finite regional cloud-shadow map fades between 20 and
120 km observer altitude. A cloud-shadow on/off comparison isolated diagonal
globe streaks to that map; changing the cloud upsampler did not solve them.
In-volume ray-marched shadows and per-sample solar transmittance remain active.
The level stores real-time skylight capture before scene registration, avoiding
an unnecessary render-proxy rebuild during the first parallel draw commands.

The renderer has finite primary/shadow sample budgets and early transmittance
termination. It uses native Unreal cloud scattering and shadows; it is an
art-directed weather model, not a meteorological fluid simulation.

## Validation

### Startup stability

Repeated source-game launches exposed intermittent failures in Nanite's parallel
shader-binding recording before the cloud probe started. DLL export offsets
resolve the first calls to `FReadOnlyMeshDrawSingleShaderBindings::SetShaderBindings`
and `FMeshDrawShaderBindings::SetParameters`. Epic documents this call chain in
[UE-367483](https://issues.unrealengine.com/issue/UE-367483). The installed 5.8
source already contains the task wait discussed there, so this is evidence of a
related path, not proof of that exact unfixed defect.

`r.Nanite.ParallelBasePassBuild=0` uses serial CPU command recording for the Nanite
base pass. Nanite geometry, materials and GPU shading remain enabled. Repeated
startup, performance and full-flight captures qualify this workaround; it does
not claim to fix the engine's separate D3D12 residency crash. Do not re-enable the
parallel path until a replacement engine build passes those checks.

### Render captures

`Tools/Tests/measure_clouds.ps1` records isolated normal-window GPU captures at
ground, cloud-layer, 14 km, 94 km and globe viewpoints. It waits for startup and
15 seconds of weather/exposure settling, then records 360 frames and a screenshot.
Compare the same output resolution, reconstruction, hour and weather. Keep a
single rendering process running. The script preserves raw CSV and logs; source
Editor `-game` has a known unrelated GameFeatureData diagnostic which remains in
the log and is not treated as a material or GPU success/failure signal.

Stationary probes do not validate reprojection motion. Full-flight cloud-layer
sequences, close rocket silhouettes, sunset/night and overcast probes must also
be reviewed before release. No claim of a universal frame rate follows from one
probe, and 2 ms targets from another game are not transferable hardware results.

The raw frame timer includes the synchronous screenshot taken during each probe;
cloud GPU pass timings are the primary comparison. No frame-time samples are
silently removed. Regional ocean shading seams remain visible in the cloud-off
reference: they are a separate world-material defect, not a cloud clipping edge.

### Measured comparison (RTX 4070 SUPER, Ryzen 5 9600X)

Output: 2560 x 1440, RT off, noon, broken-cloud preset, DLSS Quality. These are
isolated observer probes, not a promise for every phase or graphics configuration.
The cloud shape/material changes between versions; resolution and preset match.

| Observer | Previous cloud GPU | New cloud GPU | Previous / new mean FPS |
| --- | ---: | ---: | ---: |
| 94 km | 37.95 ms | 0.37 ms | 20.4 / 110.7 |
| Whole Earth | 30.76 ms | 0.47 ms | 24.3 / 124.3 |
| Within low layer | 22.73 ms | 1.64 ms | 30.5 / 102.1 |
| Ground | not captured | 1.31 ms | not captured / 118.2 |

The harder native-resolution overcast probe at 94 km measures 2.19 ms for clouds
and 67.1 mean FPS, with a 2.60 ms cloud p95. At the overcast layer top the cloud
pass costs 4.13 ms (5.14 ms p95), at 64.8 mean FPS. This keeps fully covered cases
separate from the cheaper broken-cloud view. Raw CSVs, settings, screenshots and
logs are retained in `Docs/Releases/0.1.0-alpha.9/Clouds`.

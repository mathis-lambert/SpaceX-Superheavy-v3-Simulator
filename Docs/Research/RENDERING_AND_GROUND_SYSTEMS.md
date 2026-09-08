# Rendering and ground systems research

Research date: 2026-09-08. Implementation baseline: Unreal Engine 5.8, commit `1527ddc`.

## Rendering sources and decisions to test

1. [Epic: Volumetric Cloud](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine). The project currently forces full-resolution cloud tracing (mode 3). Compare reconstructed modes 0/1 against the exact existing motion sequences, with opaque cloud intersections and the high-altitude horizon as mandatory gates. Do not use mode 2 to hide intersection defects. Test conservative empty-space skipping and material expense before reducing detail.
2. [Epic: Heterogeneous Volumes](https://dev.epicgames.com/documentation/en-us/unreal-engine/heterogeneous-volumes-in-unreal-engine). Compare the existing procedural participating medium with sparse precomputed volumes and a bounded fluid simulation. Measure voxel/lighting resolution, overlapping bounds, shadow cost, animation streaming and temporal artifacts. A box count or a material compilation success is not a smoke-quality metric.
3. [Epic: PSO Precaching](https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine). Validate hits, misses and late PSOs in a built game. Precaching is already an engine facility; adding a default CVar without measuring first-use stalls would not fulfill requirement 7.
4. [Epic: Texture Streaming Metrics](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-metrics-in-unreal-engine). Distinguish pool reservation from resident mip memory. Record wanted/resident mips during camera transitions and descent before changing pool limits or forcing high-resolution residency.

## Ground preparation fidelity

[NASA/SpaceX LC-39A Final EIS, Volume I](https://netspublic.grc.nasa.gov/main/SpaceX-SSH-LC-39A-Final-EIS-Volume-I.pdf), January 2026, describes chilled engines, spin-prime tests, static fires and fueled/unfueled rehearsals. These establish types of preparation, not the exact terminal sequence for a particular Starbase flight.

The current final-minute sequence is explicitly an estimated interactive rehearsal: terminal checks at T−60, readiness at T−30, an internal-power command at T−20, water at T−10, engine start at T−3, and a physical thrust check before mount release. Internal power is currently a command milestone, not an electrical network model. No claim is made that these timings reproduce a certified flight procedure. Public SpaceX mission pages were attempted but returned no extractable timeline; do not borrow Falcon 9 or SLS timings as Starship evidence.

The clock may hold before ignition when a known actuator fault is present. A fault after ignition or inadequate measured thrust at T-zero aborts with the mount retained. Release does not set velocity or pose. Condensation uses measured modeled vent flow; the current valve duty is estimated and does not yet replace the open thermal/ullage requirement (82).

## Required evidence

For the first ground change: cold-hold/resume and ignition-abort fixtures, a full rendered minute including home vapor and deluge, no duplicate ignition or reset across pause/input events, conservation with vent replenishment, and a complete physical recovery flight. Existing reports from the checkpoint are baseline evidence only.

## Verified engine conventions and next experiments

The installed UE 5.8 renderer provides stronger version-specific evidence than generic advice:

- `HeterogeneousVolumes.cpp` defaults the maximum camera trace to 30,000 cm. The home camera is farther away. The project now permits 12 km, covering the remote observer while retaining bounded local volume intersections.
- `HeterogeneousVolumesRayMarchingUtils.ush` integrates extinction against the local ray step. The condensation component now uses isotropic 50 cm voxels and converts estimated inverse-metre extinction by 0.5. Unequal axis scaling previously changed the relationship between optical depth and world distance. A temporary uniform-density probe confirmed the renderer and lighting path; it was removed after diagnosis.
- `StreamingManagerTexture.cpp` emits the selected texture-streaming counters in MiB, despite their engine `MB` names. These represent individual streaming quantities, not total VRAM. The new analyzer also records the RHI local-memory counter and its budget separately.

[Schlüter et al., Rendering Large Volume Datasets in Unreal Engine 5 (2025)](https://arxiv.org/abs/2504.07485) compares volume-texture, Niagara and sparse-volume approaches. Its chunk-boundary lighting problems reinforce the need to test adjoining volumes, not merely individual smoke assets. This is a scientific-visualization survey, not evidence that a particular rocket effect meets real-time quality or cost targets.

Epic's cloud documentation also describes conservative density evaluation: an inexpensive bound can skip the expensive material only where cloud density is certainly zero. Inspect the existing engine-derived graph before adding duplicate logic. A reconstructed cloud mode is being measured against mode 3; opaque silhouettes and orbital horizon images remain required before changing the shipped setting.

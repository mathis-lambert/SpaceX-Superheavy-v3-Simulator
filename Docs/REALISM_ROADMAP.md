# Realism programme

Updated 2026-09-08. This is an implementation backlog, not a claim that every item is complete. The existing working version is preserved in `Saved/Recovery/BeforeStrictPhysics` with SHA-256 verification. Player-facing text and project documentation remain English.

## Non-negotiable simulation contract

During an active flight, guidance may command engine ignition, throttle, gimbals, reaction valves and aerodynamic surfaces. It may not set vehicle position, attitude or velocity, freeze a failed vehicle, or add a corrective force with no physical source. Every external force and moment needs an attributable model and a bounded actuator or environmental cause. An unsuccessful approach must be able to miss, collide, fall, exhaust propellant or abort.

Physical connections are allowed when they represent actual hardware: stage attachment, launch hold-downs, hinges and rail contacts. They need explicit release conditions and documented mechanical assumptions. Initial-state placement, a complete simulation pause and a coordinate-origin change that preserves relative states are not flight control. Rendering interpolation must never feed back into simulation.

Approximate physics remains approximate: public sources do not provide a complete Super Heavy aerodynamic database, tank/inertia history, engine transient model or flight software. Estimated parameters must be labelled, versioned and tested for sensitivity. Passing a catch test is not evidence that these estimates match a real flight.

## Work packages and acceptance

### A. Vehicle dynamics and propulsion — first implementation priority

1. Replace direct roll assistance with forces from individual engine locations.
2. Conserve each engine's thrust-vector magnitude during gimbal deflection.
3. Use a shared engine geometry registry for simulation, nozzle animation and exhaust placement.
4. Model individual engine startup, shutdown and throttle lag; later calibrate restart limits, minimum stable burn and settling requirements.
5. Enforce gimbal travel, slew rate and engine-specific eligibility.
6. Limit delivered impulse by available propellant, including the final fraction of a timestep.
7. Represent reaction control with fixed, one-way nozzle directions and finite thrust.
8. Add valve response, pulse duration and a traceable gas budget.
9. Simulate separated Starship as an independent rigid body with mass, propellant and forces.
10. Preserve rigid-stack point velocity and angular velocity through separation; remove the scripted separation kick.
11. Replace the prelaunch frozen body with explicit physical launch hardware.
12. Keep environmental forces active after abort, fuel exhaustion and catch.
13. Keep passive aerodynamic loads on stationary grid fins after engine shutdown.
14. Model separate LOX/methane tank depletion, moving centre of mass and inertia.
15. Introduce bounded propellant slosh after the rigid-body model is validated.
16. Use consistent spherical altitude, radial gravity and local vertical definitions.
17. Add rotating-atmosphere/Earth effects with documented reference frames.
18. Run control and actuator integration at a documented physics cadence, independently of render FPS.

Acceptance: no in-flight pose/velocity writer, no unexplained control torque, no thrust without propellant, meaningful coast and separation conservation checks, and reproducible outcomes over timestep refinement.

### B. Aerodynamics and guidance

19. Replace constant fin lift slope with versioned Mach/angle-of-attack tables.
20. Include transonic behaviour, stall and fin-force saturation.
21. Sample airflow at each fin, including body angular velocity.
22. Apply body forces at a documented centre of pressure rather than assuming the centre of mass.
23. Add roll/pitch/yaw aerodynamic damping from coefficients rather than generic engine damping.
24. Add configurable wind shear, gust spectra and turbulence seeds.
25. Separate estimated sensor state from simulation truth.
26. Add IMU/GNSS/radar latency, noise and dropout scenarios.
27. Recompute landing-burn ignition from bounded available thrust and state uncertainty.
28. Replace long terminal hovering with a feasible descent corridor and reserve budget.
29. Add engine-out allocation and unreachable-target detection.
30. Add abort-to-clear-zone logic when capture constraints cannot be met.
31. Track dynamic pressure, angle of attack and structural-load margins throughout descent.
32. Validate Monte Carlo dispersions rather than tuning only one successful trajectory.

Acceptance: fins lose authority as density tends to zero; disabled actuators cannot be rescued by guidance; failures remain physically visible. Distinguish an operator's target corridor from a constraint on the actual body.

### C. Launch hardware, catch and vehicle geometry

33. Resolve the current mixture of V3 labels, three-fin geometry and earlier-flight catch references.
34. Maintain an explicit vehicle-generation manifest with evidence for each dimension.
35. Measure engine, fin, fitting and tank locations from the actual imported assembly.
36. Add mechanical support geometry at the launch mount and visible hold-down operation.
37. Give catch arms bounded actuator speed/force and physical hinge behaviour.
38. Model rail compliance and energy absorption using measured or labelled estimated parameters.
39. Report fitting contact impulse, support load distribution and overload.
40. Add collision fixtures for one-fitting contact, side impact, excessive descent speed and wrong heading.
41. Make collision simplifications inspectable without using detailed render triangles for dynamic bodies.
42. Improve Starship heatshield tiles, flap hinge covers, aft structure and engine plumbing.
43. Add weld rings, panel roughness variation, frost, thermal staining and soot tied to vehicle state.
44. Make nozzle/gimbal and grid-fin animation follow measured actuator state.

Acceptance: a misaligned vehicle is obstructed by real geometry. Catch cannot succeed simply because the centreline reaches a target.

### D. Earth and geographic scenery

45. Audit effective texture resolution on screen, texture residency and mip choice at every camera altitude.
46. Replace stretched global imagery near Starbase with georeferenced local orthophoto tiles.
47. Add seamless local/regional/global blending based on projected texel size.
48. Add terrain elevation with a consistent geodetic datum and curved horizon.
49. Replace baked shadows in satellite imagery where local 3D geometry provides lighting.
50. Add shoreline masks, tidal shallows, sandbars and wetland vegetation.
51. Improve ocean roughness, shallow-water colour, sun glint and scale-dependent waves.
52. Add atmospheric aerial perspective consistently over terrain and ocean.
53. Use streamed tiles/virtual textures with a measured VRAM budget for the 4070 Super.
54. Preserve Earth scale in the physical model; use camera and playback controls to make distances accessible.

Acceptance: local detail remains readable from low cameras, there are no tile seams or abrupt altitude fades, and higher source resolution produces a verified on-screen improvement.

### E. Starbase points of interest and scene life

55. Build a reference inventory of launch tower, mount, tank farm, integration buildings and roads.
56. Add pipe racks, valves, cable trays, vents, service platforms and access ladders.
57. Add modular fencing, gates, barriers, road markings and drainage channels.
58. Add electrical cabinets, pumps, signage and realistic maintenance equipment.
59. Improve asphalt/concrete transitions, aggregate detail, puddles and edge wear.
60. Add geographically plausible palms, scrub, marsh grasses and debris distribution.
61. Add state-driven warning beacons, obstruction lights and service lighting.
62. Add sparse service vehicles and prelaunch activity, with clear mission exclusion zones.
63. Use instancing and distance tiers so small detail does not become a draw-call problem.
64. Add POI inspection cameras with concise educational annotations.

Acceptance: details have a purpose, consistent scale, material response and deliberate distance culling. No decorative geometry overlaps the launch support surfaces.

### F. Exhaust, cryogenic vents and atmosphere

65. Separate hot exhaust, condensed water, deluge spray, dust and prelaunch venting into distinct systems.
66. Tie prelaunch vent mass flow to tank loading/conditioning state, not a permanent smoke timer.
67. Add intermittent vent jets, condensation envelopes, wind transport and dissipation.
68. Improve local participating smoke with internal density variation and self-shadowing.
69. Let plume illumination scatter through vapour without washing out the scene.
70. Make exhaust expansion depend on ambient pressure and engine state.
71. Refine nozzle cores, shock structure and turbulent mixing against actual flight footage.
72. Add pressure-driven ground outflow and obstacles to the deluge cloud.
73. Preserve a wind-advected launch trail with smooth local-to-distant transitions.
74. Correct temporal ghosting, mesh outlines against clouds and translucent velocity handling.
75. Add cloud-layer variation, physically coherent sun direction and weather profiles.
76. Add subtle optical heat distortion; never move the physical vehicle to produce camera shake.

Acceptance: no smoke in vacuum from an atmospheric-only mechanism, no detached nozzle plume, no uniform glowing fog ball. GPU cost is recorded for pad, ascent, cloud crossing and catch.

### G. Rendering, cameras and performance

77. Profile GPU passes and CPU threads in a repeatable camera/flight benchmark.
78. Prioritise volumetric-cloud cost, currently the largest measured rendering expense.
79. Evaluate cloud reconstruction modes with silhouette/ghosting comparisons.
80. Add independent native/TSR quality controls without changing simulation fidelity.
81. Investigate the official DLSS plugin for the installed UE 5.8 version; expose it only when actually available.
82. Treat frame generation as an optional presentation feature, never as a physics-rate improvement.
83. Audit redundant lights, shadow-map invalidation and material overdraw.
84. Use Nanite/HLOD/instancing where measured geometry cost justifies them.
85. Eliminate allocation, object searches and synchronous asset loads during steady-state flight.
86. Add shader/PSO preparation and prevent first-use shader stalls during launch.
87. Measure frame-time distribution, not only average FPS; maintain 1440p and 4K comparison captures.
88. Refine exposure, highlight roll-off, steel reflections and night luminance together.
89. Keep orbit/free cameras stable at high speed and altitude; test motion vectors and interpolation.
90. Add realistic long-lens tracking, optical stabilization, optional grain and restrained vibration.

Acceptance: quality comparisons use identical resolution, scene, exposure and warmed shaders. Every claimed gain includes before/after evidence; upscaling mode is disclosed.

### H. Interaction, sound and project health

91. Add a force/actuator inspector showing why the vehicle turns or misses.
92. Add guided experiments for wind, fuel reserve, engine failure and grid-fin effectiveness.
93. Add flight replay and telemetry export with versioned scenario metadata.
94. Add mission debrief explaining peak loads, propellant use, landing margins and failure causes.
95. Improve spatial engine acoustics, distance delay, sonic-boom timing and atmospheric attenuation.
96. Replace surrogate audio with licensed, attributable launch/vent/mechanical recordings where available.
97. Keep simulation, presentation and UI modules independently testable with headless simulation support.
98. Keep asset source files, imported assets, runtime Blueprints and editor authoring scripts separated.
99. Add dependency/redirector checks, reproducible asset builds and a source/license manifest.
100. Prepare browser delivery as a separate architecture decision: remote rendering versus a dedicated web renderer consuming the simulation data contract.

## Second implementation wave — world continuity and flight inspection

See [World continuity and flight inspection](WORLD_CONTINUITY.md) for implementation, provenance, reproduction steps and limits. The following work is now integrated:

- Items 45–48: common geographic shading at every scale, continuous 8K coast/Gulf source mosaics, source-derived 3DEP relief, denser globe geometry, repositioned foliage. Projected-texel selection and universal residency acceptance remain open.
- Earth atmosphere: cloud tracing reaches the flight corridor's horizon; procedural stars compose with atmospheric luminance and daylight exposure. Clouds still need a richer weather structure and better high-altitude shape detail.
- Site and effects: seven new parked service props from two authored Blender models; a dedicated 128³ vapor noise field, irregular density erosion and cold descending condensation. Surveyed multi-pad layout, driving, complete smoke self-shadowing and fluid transport remain open.
- Item 91: actual force vectors at their physical application points, a live inspector and mass-centre marker.
- Item 92: individual engine failure, fin jam, reaction-jet failure, wind and attitude-response experiments. Scripted lessons and a statistically validated failure envelope remain open.
- Viewer: 0.25–4× requested playback with adaptive effective speed; nested menu redesign; optional hardware Lumen GI/reflections.
- Chase orbit: near-zero velocity no longer determines camera heading. A full Crosswind capture and contact-noise unit tests verify stability without changing the vehicle's physical motion.

These changes do not close the full programme. Highest remaining physics priorities are calibrated aero/engine data, sensor estimation, dynamic tower mechanics and efficient terminal guidance. Highest remaining world priorities are surveyed POIs, a driveable terrain/collision architecture, better shoreline/water rendering and a richer weather/steam model.

## Starting baseline and open evidence

The third wave, [controls and cleanup](CONTROLS_AND_CLEANUP.md), fixes duplicate
input ownership and tests actual keyboard dispatch before and during flight.
Menus use a compact hierarchy and vector icons. Two detailed local volumes now
render cold condensation; wind flags and bounded service traffic animate the
site. Eleven legacy Blueprint input events, unused desktop configuration and
267 unreferenced package files were removed from active content with recovery
copies. These changes advance items 67–74 and 97–99; fluid simulation, surveyed
infrastructure and the remaining physics programme stay open.

The previous rendered Crosswind run reports a 97.1 km peak, 87.8 s landing burn and 158.2 kPa peak dynamic pressure. These are simulation outputs, not matching flight telemetry. The long terminal burn and peak load need investigation. Existing contact tests and 60/30/15 FPS flights establish a useful regression baseline but do not validate the complete physical model.

Native 1440p launch previously averaged 40.42 FPS on the user's machine. Its GPU frame averaged 23.08 ms, with volumetric clouds about 11.96 ms. DLSS was initially absent. The official UE 5.8 DLSS plugin has now been installed locally and runtime support verified on the 4070 Super. Current measurements and remaining native-rendering regression are recorded in `VALIDATION.md`; no universal FPS or identical-quality guarantee is implied.

## Research anchors

- [SpaceX vehicle specifications](https://new.spacex.com/vehicles/starship): public overall geometry and engine configuration; maintain a generation tag before using individual values.
- [SpaceX vehicle updates](https://new.spacex.com/updates): V3 has three larger fins and revised catch hardware; previous-flight photographs must not silently define V3 geometry. The Flight 9 report also motivates explicit aerodynamic-load limits.
- [NASA POST2 overview](https://www.nasa.gov/post2/overview/): independent vehicle dynamics, environment and GNC models provide a useful architectural reference.
- [NASA NESC check cases](https://nescacademy.nasa.gov/flightsim/2015/bodies): numerical verification cases, including a two-stage rocket; these are not Super Heavy aerodynamic data.
- [NASA grid-fin wind-tunnel study](https://ntrs.nasa.gov/search.jsp?R=20110013520): a methodology reference for Mach/angle-dependent fin forces, not transferable vehicle coefficients.
- [NASA rocket propulsion fundamentals](https://ntrs.nasa.gov/api/citations/20140002716/downloads/20140002716.pdf): nozzle pressure and expansion reference.
- [NASA plume/water simulation](https://www.nasa.gov/aeronautics/artemis-sls-launch-sim/): multiphase launch-environment interactions; SLS geometry and quantities do not describe Starbase.
- [Epic TSR documentation](https://dev.epicgames.com/documentation/unreal-engine/temporal-super-resolution-in-unreal-engine): reconstruction quality and history tradeoffs.
- [Official NVIDIA DLSS](https://developer.nvidia.com/rtx/dlss): plugin availability must match the actual engine version.
- [Official engine-cluster photograph](https://x.com/SpaceX/status/1976058665280078003): visual reference for engine spacing, dark nozzle interiors and aft structure. Record its vehicle generation before using it for measurements.

## First-wave delivery tracking (historical)

The first implementation wave is substantial but does not close the 100-item programme. In particular, realistic tank thermodynamics, calibrated aerodynamics, dynamic tower mechanics, a surveyed Starbase and AAA smoke are still open.

Visual review also found remaining high-altitude coverage/material boundaries and overly smooth isolated vapor billows. Higher local image resolution does not solve the global/local surface transition, and a functioning volume material is not sufficient evidence of finished smoke quality. Prioritise items 45–52 and 67–74 in the next visual pass.

| Area | Implemented in this wave | Remaining acceptance work |
|---|---|---|
| Physics | Individual engine forces; bounded gimbals/valves; fuel-limited impulse; independent Starship; explicit launch mount; radial gravity; changing tank mass properties | Fin/body aerodynamic tables, sensors, rotating frames, slosh, engine restart envelopes, external numerical check cases |
| Guidance | Engine-group hysteresis avoids consecutive-frame 3/13-engine switching | Shorter efficient terminal burn, peak-load calibration, robust failures and Monte Carlo envelope |
| Cryogenics | Physical conditioning flow, small vent reaction forces, tracked ground supply and propellant balance | Tank pressure/temperature model, intermittent conditioning schedule, water deluge dynamics |
| Earth | Sixteen verified 4,000-pixel USGS source tiles, 4,096-pixel streamed BC7 assets with stable references | Prove mip residency across all cameras; improve global imagery, surface relief and shoreline/wetland detail |
| Starbase | 3,481 instances in eight batches: pipe racks, valves, tank rails, ladders, cabinets, roof equipment and cable/fence detail | Surveyed POI layout, bespoke industrial assets, worn materials, realistic service vehicles and working clamp artwork |
| Effects | Flow-oriented 3D vapor billows, wind transport, distinct condensation/steam/trail behaviour; exhaust follows delivered engine state, including six Starship plumes | Finer cryogenic turbulence, smoke self-shadowing, obstacle-aware deluge outflow and optical heat distortion |
| Rendering | Optional official DLSS/DLAA; native/TSR controls; fog-grid compensation for reconstructed scenes; comparable GPU captures | Restore native FPS headroom, reduce cloud ray-march cost, measure 4K/VRAM and inspect temporal artifacts across the full camera network |
| Interaction | Live actuator panel under I: thrust, gimbals, fins, mass centre, air load, gas and vent flow; engine indicators reflect delivered thrust | Force vectors, guided failures, replay, debrief and controllable mission experiments |
| Structure | Flight mass/propulsion/staging/ground systems separated; renderer owns Blueprint visual commands; source/asset/vendor provenance retained | Further decouple camera state from the director; formal versioned simulation snapshot for web/replay consumers |

Verified references and image annotations: [visual direction](VISUAL_REFERENCES.md). Physical assumptions and exceptions: [flight model](FLIGHT_MODEL.md).

- [x] Current source/configuration/tools/docs snapshot verified (162 files).
- [x] Code audit identified direct roll assistance, kinematic upper stage, frozen prelaunch and passive-force gaps.
- [x] Research and 100-item programme recorded.
- [x] First strict-physics implementation and conservation checks.
- [x] Full flight/contact regression after actuator changes.
- [x] Earth, site and VFX first-pass improvements with screenshots; unresolved visual limitations recorded above.
- [x] Rendering benchmark and supported reconstruction controls; native performance remains an open target.
- [x] Updated evidence and remaining-work report.

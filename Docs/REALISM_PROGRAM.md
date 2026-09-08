# Realism program — 150 requirements

Baseline: `1527ddc`. This program preserves the complete user request of 8 September 2026, including the additional launch preparation and home-screen vapor requirements. It supersedes the scope of the earlier 100-item roadmap, without treating previously implemented approximations as proof of completion.

Unchecked means pending, in progress, or insufficiently verified. A requirement is checked only when its implementation and acceptance evidence are linked here. A successful nominal capture does not prove aerodynamics, robustness, rendering quality, or the remaining requirements. Prototype passes and partial implementations remain unchecked.

## Acceptance rules

- Performance: record resolution, hardware, reconstruction, weather, camera, full-mission phase, frame-time percentiles, GPU passes and resident memory. Compare matching moving views before/after. Do not count lowering visible quality as an optimization.
- World and rendering: inspect ground, low flight, cloud crossings, 73–78 km, apogee and descent, with day/night and relevant cameras. Record source imagery, scale, mip residency, seams and measured cost.
- Physics: use physical actuation and contacts, documented units and assumptions, independent conservation/convergence fixtures, and varied flight scenarios. A simulated launch mount may restrain the vehicle only while physically connected; flight and catch must not pose or snap it.
- Interface: exercise actual input dispatch, focus, pause, restarts, keyboard layouts, controllers and viewport sizes. Rendered screenshots prove appearance; direct method calls alone do not prove controls.
- Data and maintainability: every important parameter needs a source or explicit estimate. Scripts must reproduce current assets, and packaging/reference audits must establish dependency completeness.
- Delivery: attach fresh evidence to the exact tested source/content state. Do not infer success from old reports, compile success alone, or a manifest assertion.

## Performance (1–12)

- [x] 1. Optimize volumetric clouds, currently the largest measured GPU pass. Inactive storm shader work removed with full-resolution mode 3 preserved; [measured 1440p gains and paired visual scope](Validation/REALISM_GROUND_AND_CLOUDS.md#inactive-storm-shader-checkpoint).
- [x] 2. Profile ignition, cloud crossings, entry and capture throughout a full mission. [Evidence](Validation/REALISM_GROUND_AND_CLOUDS.md#full-mission-timing).
- [x] 3. Track slow-frame percentiles and stalls in addition to average FPS. [Evidence](Validation/REALISM_GROUND_AND_CLOUDS.md#full-mission-timing); independent analyzer fixtures and raw captures retain startup and tail frames.
- [ ] 4. Establish separate budgets for clouds, smoke, lighting, shadows, water and reconstruction.
- [x] 5. Measure actual resident video memory, including Earth textures. [Evidence](Validation/REALISM_GROUND_AND_CLOUDS.md#full-mission-timing); local RHI memory and texture-streaming counters are separate. Camera-specific residency remains item 13.
- [ ] 6. Progressively preload resources for upcoming views.
- [ ] 7. Prepare shaders and PSOs before first visible use.
- [ ] 8. Remove unnecessary shadow invalidations.
- [ ] 9. Adapt small details to projected size with unobtrusive transitions.
- [ ] 10. Remove object searches, allocations and synchronous loads during flight.
- [ ] 11. Compare controlled 1080p, 1440p and 4K runs.
- [ ] 12. Verify optimizations with matching before/after moving captures.

## Earth, terrain and ocean (13–24)

- [ ] 13. Audit the Earth texture resolution actually displayed by each camera.
- [ ] 14. Improve projected-size mip and detail selection.
- [ ] 15. Prefetch detailed imagery before descent and camera changes.
- [ ] 16. Harmonize photographic source color and seasonal differences.
- [ ] 17. Remove baked photographic shadows conflicting with dynamic light.
- [ ] 18. Refine local dunes, embankments, drainage and depressions.
- [ ] 19. Improve coastline geometry and detail.
- [ ] 20. Add shallow water, sandbars and water color variation.
- [ ] 21. Combine close ripples and distant swell across wave scales.
- [ ] 22. Improve sun glints, foam and water/land transitions.
- [ ] 23. Provide terrain collisions suitable for ground exploration.
- [ ] 24. Test ground/region/globe joins over all accessible altitudes.

## Atmosphere and light (25–34)

- [ ] 25. Build coherent clear, overcast, coastal-haze and strong-wind profiles.
- [ ] 26. Calibrate exposure, lights and materials together for day and night.
- [ ] 27. Reduce repetitive cloud shapes and placement.
- [ ] 28. Add distinct moving cloud layers.
- [ ] 29. Improve cloud tops and continuity to the horizon.
- [ ] 30. Refine dawn/dusk without artificial saturation.
- [ ] 31. Harmonize haze over land and ocean.
- [ ] 32. Prevent abrupt exposure shifts during camera rotation.
- [ ] 33. Make star visibility consistent with exposure.
- [ ] 34. Compare hardware/software lighting for shadows, reflections and volumes.

## Flames, smoke and vapor (35–48)

- [ ] 35. Benchmark detailed smoke methods in a controlled comparison scene.
- [ ] 36. Remove the stacked-ball appearance of ground vapor.
- [ ] 37. Make deluge plumes spread along the ground.
- [ ] 38. Make flows divert around major launch structures.
- [ ] 39. Combine large turbulent rolls with smaller torn structures.
- [ ] 40. Distinguish cold condensation, deluge vapor, dust and hot gas.
- [ ] 41. Make cryogenic vent duty respond intermittently to tank conditioning.
- [ ] 42. Improve condensation descending along the shell before wind advection.
- [ ] 43. Refine internal shadows and transmission through thin vapor.
- [ ] 44. Prevent engine light from uniformly tinting whole clouds orange.
- [ ] 45. Refine plume expansion with ambient pressure and engine operation.
- [ ] 46. Improve flame cores, mixing and temporal variation.
- [ ] 47. Make the ascent trail persist and deform with wind at long range.
- [ ] 48. Correct temporal ghosts, volume boundaries and transparency artifacts.

## Starbase (49–60)

- [ ] 49. Establish a dated reference plan of buildings, roads, tanks and facilities.
- [ ] 50. Match major POI locations and proportions to the reference.
- [ ] 51. Rework concrete, asphalt, joints, curbs and ground transitions.
- [ ] 52. Add localized wear, tire marks, grime, repairs and wet patches.
- [ ] 53. Detail visible pumps, exchangers, cabinets and electrical equipment.
- [ ] 54. Improve pipes, supports, couplings, valves and diameter transitions.
- [ ] 55. Add cable trays, platforms, stairs, handrails and maintenance access.
- [ ] 56. Develop drainage ditches, gutters and drains.
- [ ] 57. Make gates and barriers visibly operational.
- [ ] 58. Connect site activity to preparation, countdown and recovery.
- [ ] 59. Improve service-vehicle wheels, steering, suspension and routes.
- [ ] 60. Support multiple configurable pads from a documented plan.

## Vehicles and equipment (61–70)

- [ ] 61. Choose one mechanically consistent generation of vehicle.
- [ ] 62. Separate verified dimensions from estimates.
- [ ] 63. Refine Starship silhouette, flaps, fairings and aft structure.
- [ ] 64. Refine thermal protection tiles and local variation.
- [ ] 65. Correct repeated, stretched or uniform metal textures.
- [ ] 66. Add welds and roughness variation at multiple scales.
- [ ] 67. Evolve frost, condensation, soot and heating marks during flight.
- [ ] 68. Refine visible engine nozzles and plumbing.
- [ ] 69. Replace simplified RCS blocks with correctly oriented nozzles.
- [ ] 70. Validate pivots, normals, LODs and collisions for moving parts.

## Fundamental physics (71–84)

- [ ] 71. Decouple guidance and actuator cadence from rendering.
- Current work: value-only engine bank and analytic valve impulse; actual Chaos/control cadence is measured. The callback transfer remains open; see [propulsion and scheduling evidence](Validation/PROPULSION_AND_SCHEDULING.md).
- [ ] 72. Verify numerical convergence as the integration step decreases.
- [ ] 73. Extend mass, momentum and energy conservation checks.
- [ ] 74. Add Earth rotation and a consistent atmospheric reference frame.
- [ ] 75. Extend the documented high-altitude atmosphere model.
- [ ] 76. Add reproducible wind shear, gusts and turbulence.
- [ ] 77. Replace simple aerodynamic coefficients with documented tables.
- [ ] 78. Include transonic behavior, grid-fin stall and saturation.
- [ ] 79. Compute local flow at each fin including body rotation.
- [ ] 80. Vary center of pressure with attitude and flight conditions.
- [ ] 81. Add a controlled propellant slosh model.
- [ ] 82. Model tank temperature, pressure, ullage and pressurization.
- [ ] 83. Model engine operating and restart limits.
- [ ] 84. Add thermal/structural limits with physical consequences.

## Flight computer and trajectory (85–96)

- [ ] 85. Rework the excessively long terminal braking burn.
- [ ] 86. Calculate landing margins from available thrust and fuel.
- [ ] 87. Improve boostback impact prediction.
- [ ] 88. Account for consumption and uncertainty in boostback optimization.
- [ ] 89. Separate onboard estimates from exact simulated state.
- [ ] 90. Add sensor noise, delay and drift.
- [ ] 91. Simulate temporary position/altitude measurement loss.
- [ ] 92. Improve allocation after engine/actuator failure.
- [ ] 93. Detect unreachable targets before reserves are exhausted.
- [ ] 94. Add an abort decision and trajectory toward a clear area.
- [ ] 95. Reduce unnecessary engine, gimbal, fin and RCS activity.
- [ ] 96. Evaluate guidance over seeded randomized flight ensembles.

## Tower and capture (97–106)

- [ ] 97. Replace simplified arm motion with force/speed-limited mechanisms.
- [ ] 98. Represent arm joints and degrees of freedom.
- [ ] 99. Add rail compliance and energy absorption.
- [ ] 100. Measure each fitting and arm load separately.
- [ ] 101. Allow one-sided support, sliding and tipping.
- [ ] 102. Model overload deformation or failure.
- [ ] 103. Improve contact friction and small rebounds.
- [ ] 104. Test excessive speed, tilt and incorrect heading at capture.
- [ ] 105. Match visible launch clamps to mechanical state.
- [ ] 106. Inspect tower collisions and forces in debug mode.

## Controls, cameras and interface (107–120)

- [ ] 107. Add key rebinding with conflict detection.
- [ ] 108. Test AZERTY/QWERTY and live layout changes.
- [ ] 109. Provide complete gamepad navigation.
- [ ] 110. Separate free-camera, orbit and zoom sensitivity.
- [ ] 111. Change focus without losing framing.
- [ ] 112. Save and recall personal camera positions.
- [ ] 113. Improve long-lens tracking, stabilization and depth of field.
- [ ] 114. Prevent camera penetration of terrain, tower and vehicles.
- [ ] 115. Add simultaneous booster/Starship views and a trajectory minimap.
- [ ] 116. Add photo mode with exposure, focal length and high-resolution export.
- [ ] 117. Adapt menus to small, 4K and ultrawide displays.
- [ ] 118. Set HUD and menu scales independently.
- [ ] 119. Refine selected, active, disabled and hover states.
- [ ] 120. Keep educational information optional and available on demand.

## Interactivity and learning (121–131)

- [ ] 121. Add mission debrief with fuel, loads, precision and outcome reasons.
- [ ] 122. Record the flight computer decision timeline.
- [ ] 123. Graph thrust, speed, altitude, dynamic pressure and reserves.
- [ ] 124. Link force vectors to their source and magnitude.
- [ ] 125. Schedule a failure by time or altitude.
- [ ] 126. Support progressive, intermittent and combined failures.
- [ ] 127. Compare two flights with identical initial conditions.
- [ ] 128. Provide engine-out, crosswind, fin-jam and low-reserve experiments.
- [ ] 129. Build replay from recorded physical states.
- [ ] 130. Seek within replay without rerunning physics.
- [ ] 131. Manual control must use the same physical actuators and limits.

## Sound (132–138)

- [ ] 132. Replace provisional audio with higher-quality attributed recordings.
- [ ] 133. Distinguish ignition, spool-up, shutdown and relight.
- [ ] 134. Add vents, pumps, mechanisms, vehicles and coastal ambience.
- [ ] 135. Calculate sound propagation delay from distance.
- [ ] 136. Improve attenuation, low frequencies and Doppler.
- [ ] 137. Derive supersonic sound events from trajectory geometry.
- [ ] 138. Provide speaker, headphone and recording mixes.

## Architecture and validation (139–150)

- [ ] 139. Formalize a common state consumed by rendering, HUD, replay and exports.
- [ ] 140. Extract responsibilities from the mission director.
- [ ] 141. Centralize units, conversions, frames and geometry conventions.
- [ ] 142. Version vehicle, weather and guidance profiles.
- [ ] 143. Source or explicitly qualify every important parameter.
- [ ] 144. Keep Blueprint interfaces explicit and graphs limited to their role.
- [ ] 145. Automate broken-reference, duplicate and unused-asset checks.
- [ ] 146. Make generation/import reproducible without rebuilding old prototypes.
- [ ] 147. Add physical tests independent of nominal capture scenarios.
- [ ] 148. Automate visual regression detection on reference views.
- [ ] 149. Test long sessions for memory, reloads and stability.
- [ ] 150. Attach a validation report to each deliverable version.

## Additional explicit requirements

- [x] A1. Visible, continuous propellant condensation on the home-screen rocket. [Day/night and home evidence](Validation/REALISM_GROUND_AND_CLOUDS.md#ground-sequence-and-condensation).
- [ ] A2. A progressive countdown with meaningful real-world preparation activities and verified liftoff interlocks.
- [ ] A3. Consult authoritative Unreal documentation and rendering literature for planet scale, rocket flames and high-quality smoke, with measured implementations.
- [ ] A4. Maintain coherent Unreal architecture; remove unstable legacy paths and unnecessary compatibility instead of layering over them.

## Work log

Ground launch sequencing, physical thrust interlocks and condensation are implemented and tested. A full nominal rendered profile covers ignition through passive rail support. Paired moving captures are complete; reconstructed cloud mode 1 remains rejected after sampled edge artifacts. The mode-3 material now removes inactive storm calculations, with a completed timing run and partial visual review. [Current validation and limitations](Validation/REALISM_GROUND_AND_CLOUDS.md). Thermal tank preparation, electrical systems and moving clamps remain open, as do the unchecked requirements above.

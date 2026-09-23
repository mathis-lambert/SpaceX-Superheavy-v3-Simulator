# Starbase Flight Simulator - 0.1.0-alpha.11

Windows x64 standalone Development build, Unreal Engine 5.8.

## Start and controls

Open `Windows/SuperHeavySim.exe` and keep its entire distribution together.
The Unreal Editor is not required. If runtime libraries are missing, run
`Windows/Engine/Extras/Redist/en-us/vc_redist.x64.exe`.
Use a DirectX 12 / Shader Model 6 GPU with current drivers. Hardware qualification
is currently limited to the development RTX 4070 SUPER machine.

Select Launch to choose a mission. The pointer remains visible: hold right mouse
to orbit and left-click vehicle parts to inspect or introduce failures.
Tab selects cameras; F enables free flight; I displays forces; L opens the flight
computer. Escape closes inspection before opening pause/settings. J/K changes
requested playback speed; effective speed is limited by the physics budget.
Mission and Restart remain available after a failed recovery or water contact.
Restart requires confirmation. Camera inversion and preferences are saved.

## Current capabilities

- Physical launch, independent stage separation, boostback, unpowered coast,
  atmospheric control and tower recovery with individual actuator limits.
- Contextual engine/fin/RCS failures, scheduled outages, health-aware allocation,
  predicted trajectories and estimated alternate recovery objectives.
- Physical tower arms and compliant rails; propulsion shutdown on fitting support.
- Distributed buoyancy and drag after water contact; the intact booster can settle
  on its side without a prescribed floating pose.
- Spherical Earth, coastal terrain, service traffic, night lighting, configurable
  weather and photographic settings with saved looks and geographic solar time.
- Layered propulsion audio with distance delay, volumetric effects and startup
  preparation of scene resources. Optional NVIDIA DLSS/DLAA and hardware Lumen.

## Limits

The model uses estimated aerodynamic, propulsion and mechanical parameters;
it does not reproduce confidential SpaceX flight data. Complete-return numerical
convergence is not established. The planner is not a certified six-degree-of-
freedom optimal controller, and recoveries may fail.

Marine contact assumes a sealed, intact booster at mean sea level; breakup,
flooding and individual wave forces are absent. Tower hardware values are
estimates and the carriage is an ideal fixed bearing. Imagery and site layout
are not a current survey. Cloud/smoke quality and performance still need work;
no universal FPS or hardware compatibility claim is made.

See `FLIGHT_MODEL.md` for the physical boundary. Build provenance is recorded in
`build-manifest.json`; distributed archives have SHA-256 checksums. Source and
vendor attributions accompany the distribution in `ThirdParty/` (in the source
repository, `Docs/Credits/`). Engine and vendor licenses remain applicable.

# Flight model

Updated 2026-09-08. Chaos integrates the rigid bodies. Guidance commands bounded actuators; presentation reads their resulting physical state. Starship becomes an independent physical body at separation. The default return near 97 km is an estimated scenario, not a telemetry replay.

## Configuration and units

`DA_RecoveryMission` configures the mission in metres, kilograms, seconds and newtons. The Unreal boundary converts position/force to centimetre units and inertia/torque to squared units. Spherical altitude, radial gravity and local vertical use the geographic Earth. The atmospheric layers have their own standard geopotential reference radius; that is not a second rendered Earth radius.

`RecoveryAtmosphere` implements seven layers of the [US Standard Atmosphere 1976](https://ntrs.nasa.gov/api/citations/20050207438/downloads/20050207438.pdf) through approximately 86 km geometric altitude, followed by a simplified continuation. Dynamic pressure is `0.5 × density × air-relative speed²`. Atmospheric rotation is not yet modelled and navigation reads perfect simulation truth. Public sources do not supply complete Super Heavy aerodynamic, tank/inertia, engine-transient or GNC data; unavailable parameters remain configurable estimates.

## Engines and attitude control

`RecoveryPropulsion.cpp` resolves the 33 actual Blueprint engine mounts into a shared geometry registry once. Each engine applies force at its mount. Gimbal deflection preserves thrust magnitude and obeys configured travel, slew rate and response time. Lateral-force allocation produces pitch, yaw and roll through engine lever arms. There is no additional direct engine roll torque. Saturation can prevent a requested attitude from being achieved.

Startup/throttle response uses bounded first-order response; shutdown closes over a finite interval. Specific impulse varies between configured sea-level and vacuum values. These estimates are not measured Raptor performance curves. Delivered impulse is limited by remaining fuel, including the final fraction of a timestep. Consumption follows `mass flow = thrust / (specific impulse × standard gravity)`, consistent with the [specific impulse definition](https://www.grc.nasa.gov/www/BGH/specimp.html).

`RecoveryPropulsionModel` integrates booster valve impulse analytically over each
held-command interval. Endpoint thrust drives artwork; step impulse determines
the mean mechanical force and fuel consumption. The value-only engine-bank
model has no Unreal object access. `RecoveryDynamicsModel` evaluates the booster
engine bank, attitude loop, RCS, grid fins, conditioning flow and mass properties
at every actual Chaos substep through `RecoveryPhysicsComponent`.
`RecoveryGuidanceModel` evaluates navigation, ballistic prediction and flight
commands in that same callback. Ground sequencing, mechanical event delivery,
tower arms and upper-stage dynamics still use the game frame. See the
[current solver guidance boundary and evidence](Validation/SOLVER_GUIDANCE.md).

Landing guidance uses separate switch-down/switch-up thresholds for the three- and thirteen-engine groups. This prevents consecutive-frame command chatter near one thrust threshold. It does not add thrust or constrain the body. Hardware restart counts, settling requirements and minimum stable operating duration still need a calibrated model.

Reaction control uses six estimated pod locations, each representing two opposing one-way ports. The allocator closes the existing valve before reversing and applies forces at their locations. Valve response, maximum force and a separate gas supply bound authority. The six displayed pods are simplified artwork; detailed paired-nozzle geometry remains unfinished.

The controller uses current analytical inertia and includes the gyroscopic term. Chaos gyroscopic torque is enabled. Passive body/grid-fin aerodynamic loads remain after engine shutdown and contact; generic rigid-body damping is zero. Fin lift and body drag remain simplified coefficient models. Local angular airflow, transonic tables, pressure-centre motion and aeroelasticity are still work items.

## Mass, tanks and ground conditioning

`RecoveryMassProperties` combines axial shell/tank parts with the parallel-axis theorem. LOX and methane draw down at a configured mixture ratio: fill height, centre of mass and inertia change with remaining mass. Dry structure, reaction gas and attached upper stage contribute separately. Tank positions, liquid densities and dry structure distribution are estimates. This quasi-static distribution model does not solve internal flow, ullage pressure, settling or slosh.

The prelaunch stack is a live body retained by an explicit mount constraint with estimated break limits and no projection. Ignition precedes release; release requires a countdown threshold and sufficient thrust. This represents physical hold-down hardware. Detailed visible clamp mechanisms remain unfinished. No hold-down constraint exists during free flight or catch.

`RecoveryGroundSystems` publishes ground-supply and deluge commands. The solver
model integrates two conditioning vents with bounded valve response, propellant loss and small reaction forces. Connected ground supply replaces vented mass; disconnect ends replenishment and valves close. The tracked balance is:

`initial propellant + ground supply = remaining propellant + main-engine consumption + vented propellant`.

The vapor renderer reads actual conditioning flow. Its condensation, transport and dilution are approximate visual models, not a thermodynamic tank or multiphase CFD solver.

## Stage separation

The attached upper stage contributes to stack mass properties. `RecoveryStageDynamics` initializes independent Starship at the inherited rigid-stack location, orientation, point velocity and angular velocity. Booster velocity is transported to its new mass centre. This changes one rigid assembly into two bodies; it is not a guidance correction. There is no scripted separation kick or prescribed subsequent trajectory.

Linear and angular momentum mismatches are recorded before subsequent engine integration. Starship then experiences its own gravity, approximate aerodynamics, six engine forces and fuel consumption. Its renderer and six exhaust plumes follow that independent body. Uniform axial inertia and symmetric fixed thrust directions remain approximations. Orbital guidance, distinct vacuum/sea-level performance, hot-stage impingement and detailed Starship tank modelling remain unfinished.

## Recovery and contact

1. Condition the stack, ignite 33 engines and release launch hardware.
2. Ascend, shut down, separate and command a return attitude.
3. Perform boostback on 13 engines using an updated ballistic impact estimate.
4. Coast unpowered and descend, transitioning reaction jets to grid fins.
5. Ignite landing engines from descent energy and available thrust.
6. Approach through the tower opening with a position/heading/tilt target corridor.
7. Shut down on acceptable fitting support contact and settle on the rails under gravity.

The corridor changes guidance demands, not vehicle pose. The controller can fail. The long terminal burn and high dynamic pressure in current successful trajectories still need calibration; a successful catch alone does not validate them.

Measured catch fittings sit approximately at `(±4.99, 0.0079, 62.7978)` metres from the booster base. Required heading is 90° relative to the tower. Launch and catch share an axis 24 m in front of the tower. The reference mesh, three-fin arrangement and generation labels still need one consistent documented vehicle-generation decision.

Two fitting collision boxes are welded into the booster body. Arm and rail colliders obstruct it. A slow, correctly aligned first support contact shuts off engine and attitude commands; residual thrust follows finite valve closure. Capture requires both supports to settle, followed by eight seconds of passive support. There is no pose snap, velocity reset, active holding force or added catch constraint. Incorrect alignment and lateral impacts remain collisions.

The tower is static and arm positioning remains kinematic. Force-limited arm actuators, rail compliance, overload damage and bearing stresses are not yet modelled. This boundary is explicit: the flying vehicle uses physical forces, while complete mechanical fidelity of the tower remains unfinished.

## Verification and code boundary

The same force/mass model runs without rendering. Unit checks cover the final-fuel impulse budget, engine moment allocation and separation mass/first-moment/inertia consistency. Flight reports include actuator limits, separation momentum errors, ground-supply balance and passive catch results. Integration tests vary scenario and timestep; see [validation evidence](VALIDATION.md).

Only initialization/reset, scenario fixtures and separation initial-state transport may set physical state directly. Presentation may set decorative meshes, camera transforms and Blueprint actuator artwork. It does not command the flight body. The [NASA POST2 architecture](https://www.nasa.gov/post2/overview/) and [NESC two-stage check cases](https://nescacademy.nasa.gov/flightsim/2015/bodies) are methodological references; this project has not completed their independent validation suite.

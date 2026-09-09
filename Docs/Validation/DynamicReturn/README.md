# Dynamic return evidence

The eight `DynamicReturn15` flight reports pass the stricter landing contract:
under 45 seconds of landing thrust, incoming body speed below 1.5 m/s, less than
8 seconds below 5 m/s before contact, no structural collisions, front ingress,
both solver rail supports and eight seconds of passive support with engines off.
`comparison.json` additionally compares each run with the retained baseline.

- Nominal, Crosswind and increased-mass Offset: 60 and 15 Hz game cadence, with
  the normal 120 Hz physical clock.
- Crosswind with the injected variable-frame/stall clock: 120 Hz physics.
- Crosswind with 240 Hz physics: a refinement spot check, not proof of full-flight
  numerical convergence. Its 31.92 s thrust time compares with 31.58 s at 120 Hz.
- `model-tests.json`: all 23 model/presentation tests passed without warnings.
- Four unpowered contact fixtures verify centered and longitudinally offset
  support, rejection of wrong heading, and recognition of a structural side hit.

Across these eight runs, landing thrust lasts 30.16–32.21 s, incoming body speed
is 0.70–1.03 m/s and the slow interval lasts 4.48–5.42 s. The stricter matrix
requires zero structural contact notifications, in addition to the solver support
contract. Delayed hit diagnostics use a consistent component transform.

`*-source.json` records file hashes, engine version, module DLL and Git state at
test time; user preferences were omitted from the committed copies. Raw JSON,
solver cadence reports and compressed baseline/new Crosswind CSV traces are
retained here. Development iterations remain under ignored `Saved/Recovery`.
The `Baseline` reports predate this guidance change. Packaged executable records
live separately under `Docs/Releases/0.1.0-alpha.4`.

From the project root:

```powershell
./Tools/Runtime/build_simulator.ps1
./Tools/Tests/test_physics_models.ps1 -Prefix DynamicReturnModels
./Tools/Tests/test_contact_fixtures.ps1 -Prefix DynamicReturnContact
./Tools/Tests/test_physical_recovery.ps1 -Prefix DynamicReturn -Cadences 60,15
./Tools/Tests/test_physical_recovery.ps1 -Prefix DynamicReturnJitter -Scenarios Crosswind -Cadences 60 -JitterClock
./Tools/Tests/test_physical_recovery.ps1 -Prefix DynamicReturn240 -Scenarios Crosswind -Cadences 60 -PhysicsHz 240
python Tools/Tests/analyze_dynamic_return.py Saved/Recovery/DynamicReturn_Crosswind_60.json --output Saved/Recovery/DynamicReturn-comparison.json
```

# Validation

## Repeatable checks

Hosted CI checks Python syntax and regression fixtures, PowerShell syntax and
tracked files that violate `.gitignore`. Run the commands in
[Tests](../Tests/README.md) locally. The tool suite does not validate Unreal graphics.

Build the editor, then use `run_model_tests.ps1` for model automation,
`test_return_scenarios.ps1` for flight scenarios and `test_viewer.ps1` for
assets, interface and rendered flights. Dedicated contact, resilience, emergency,
ground-sequence and marine tests live beside them. Packaged acceptance uses
`test_windows_package.ps1`; publication only follows passing release gates.

Reports, CSVs, logs and captures belong in ignored `Saved/Recovery` and release
artifacts, not source documentation. Keep comparison baselines externally and
pass them explicitly to analysis tools. Old measurements remain in Git history;
they are not acceptance evidence for a newly changed build.

## Interpreting results

A successful capture does not prove flight accuracy or numerical convergence.
Compare matched physical times, timestep refinements and actuator/fuel budgets.
For rendering, match camera, weather, reconstruction and output resolution;
measure frame-time distributions after warmup and inspect temporal behavior.
Static screenshots alone cannot prove smoothness or a performance improvement.

The asset audit on 2026-09-23 found 171 reachable packages and no orphans.
The full Unreal asset/Blueprint audit also completed successfully. No runtime
assets were removed by that audit. These checks do not replace a full cooked
release test or validation on additional hardware.

See [CI and releases](CI_RELEASES.md) for runner setup and retained build artifacts.

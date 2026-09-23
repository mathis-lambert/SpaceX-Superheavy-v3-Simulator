# Repository working agreement

Build a maintainable, physically driven Unreal simulator with a polished presentation.
Keep one implementation of each behavior. Finish replacements by migrating consumers
and removing the superseded code, assets and configuration in the same change.

## Start here

- Inspect Git status and the current branch before editing; preserve unrelated work.
  Start new work from current `main` after a merged PR, not its old feature branch.
- Read [project structure](Docs/PROJECT_STRUCTURE.md), then only the relevant guide:
  [tools](Tools/README.md), [asset authoring](Docs/AUTHORING.md),
  [tests](Tests/README.md), [validation](Docs/VALIDATION.md) or
  [CI/releases](Docs/CI_RELEASES.md).
- Search existing implementations, callers and tests before adding a file or dependency.
  Prefer improving the owning component/tool over adding another wrapper or patch pass.

## Architecture and Unreal conventions

- `Recovery/Flight` owns simulation; `Presentation` consumes its state; `Interface`
  owns interaction. Presentation must not move the physical vehicle to make a flight
  succeed. Guidance commands physical actuators; capture uses physical contacts.
- Keep reusable model calculations independent of UI and rendering. Centralize shared
  geometry/unit conventions and asset references in their existing owners.
- Use Unreal actor/component lifecycles, explicit tick dependencies and appropriate
  UObject ownership (`UPROPERTY`/`TObjectPtr` or weak references as appropriate).
  Clean up delegates, timers and spawned objects with their owner.
- Follow Epic naming/reflection conventions and the repository's `.clang-format`.
  Include what a translation unit uses, keep implementation headers private, and
  declare only necessary `Build.cs` dependencies. Editor APIs belong in the editor
  module. Add a module only for a real dependency or lifecycle boundary.
- Split files by cohesive responsibility when needed; do not create forwarding-only
  classes or generic utility dumping grounds to meet an arbitrary line limit.
- Blueprints own assembly/configuration and visual actuator events; native code owns
  flight logic. Keep graphs readable, compile them after changes, and remove unused
  nodes/components rather than disabling obsolete systems at startup.
- Keep code, documentation and in-product text in English. Reuse existing UI styles,
  input definitions and menu controls. Preserve visual quality during cleanup.

## No accumulating scaffolding

- Do not retain legacy/alternate implementations, speculative compatibility shims,
  silent fallback behavior or permanent one-off fixes. Fix the owning system. If an
  external limitation genuinely requires an exception, document the precise reason,
  supported scope and removal condition beside it; do not hide the limitation.
- Put probes, migration scripts, local backups and reports in ignored `Saved/Recovery`.
  Do not commit agent journals, task summaries, duplicate plans or machine settings.
- A maintained tool needs a repeatable operation, concrete inputs/output and a current
  consumer documented in `Tools/README.md`. Extend shared helpers and retire replaced
  tools/callers together. Do not add a new script for every work session.
- Update the existing guide when behavior changes. Keep this file a short working
  agreement, not a second architecture manual or an inventory of transient counts.

## Assets and performance

- Committed Unreal packages are authoritative for builds. External `../ArtSource`
  inputs are for authoring; packaging must not depend on regenerating the world.
- Use Unreal import/reimport and reference-aware moves. Before deletion, inspect hard,
  soft and code references with `Tests/Unreal/Assets/inventory_dependencies.py` in
  Unreal. Preserve non-UObject runtime data, licenses and provenance as well.
- Close processes holding affected packages before mutation; keep verified backups
  for destructive binary edits. Validate redirects/references after moves, compile
  affected Blueprints and run the asset audit. A text search alone cannot prove a
  Blueprint member or binary asset unused.
- Keep original source data, caches and generated reports out of Git; respect
  `.gitignore` and `.gitattributes`/LFS. Required vendor runtime binaries are not junk.
- Measure CPU/GPU/frame-time and memory costs on comparable scenarios before claiming
  an optimization. Avoid per-frame discovery/loading and unnecessary ticks; use
  bounded effects and instancing where appropriate. Review rendered output alongside
  timings; do not silently lower quality to claim a performance win.

## Validation and delivery

Run PowerShell commands from the repository root. Unreal scripts accept `-EngineRoot`;
use the project's engine version and installed prerequisites, not a new hardcoded path.

| Changed area | Minimum relevant validation |
| --- | --- |
| Documentation only | Check paths, commands and `git diff --check`; no engine build needed |
| Python tooling | `python -m unittest discover -s Tests/Python -p 'test_*.py' -v` and `python -m compileall -q Tools Tests`; execute changed authoring tools in their real environment |
| Native code/reflection | `./Tools/Runtime/build_simulator.ps1`; `./Tests/Unreal/Automation/run_model_tests.ps1` for model/physics changes; validate the game target if module boundaries change |
| Flight/actuators/contacts | Native tests plus the relevant scenario runner under `Tests/Unreal/Flight` |
| Assets/cameras/UI/effects | `./Tests/Unreal/Presentation/test_viewer.ps1` with appropriate documented scope, plus visual review of fresh captures |
| Packaging/releases | Follow `Docs/CI_RELEASES.md`; validate the actual package before publication |

- Add regression coverage for critical behavior and reproduced bugs in the existing
  test domain. Use Unreal Automation for native models; do not duplicate the physics
  in Python. Keep runtime audit harnesses under `Diagnostics`, not native unit tests.
- Never weaken assertions or replace a failed run with stale evidence. Report missing
  prerequisites and unexecuted checks explicitly. Hosted tooling CI is not an Unreal
  compile, rendered-flight test or successful release upload.
- Before delivery, inspect the complete diff and `git ls-files -ci --exclude-standard`.
  Remove temporary tracked files, obsolete callers and accidental binary churn.
  Summarize behavior changes, checks and remaining limitations in the PR.
- Preserve other contributors' changes. If work is shared, agree on file ownership;
  serialize Unreal asset mutations, editor builds and scene edits in a shared checkout.
- Commit/push within the requested scope. Merging, tagging or publishing a release
  requires that action to be requested; never bypass the existing release gates or
  commit credentials. A completed code change is not automatically a release.

## Reference conventions

- [AGENTS.md discovery and scope](https://developers.openai.com/codex/guides/agents-md)
- [Epic C++ conventions](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [Unreal modules and dependencies](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-modules)

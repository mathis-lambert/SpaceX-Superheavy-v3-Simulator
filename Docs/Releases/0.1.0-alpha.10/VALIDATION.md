# Alpha 10 validation

Source: `3babd966c077d2ffd1866cf571e504180cc4d9f5`, Unreal Engine 5.8.2,
Windows x64 Development. Built and tested on 2026-09-18.

This release contains the first integrated pass of the twelve visual-renewal
workstreams. Scope, replacement decisions and remaining art-direction work are
recorded in `Docs/VISUAL_RENEWAL.md`.

- All 32 Recovery automation tests passed without warnings.
- The asset audit resolved 169 dependencies; the corrected inventory found no
  unreachable packages after seven obsolete packages were removed.
- A rendered Crosswind flight passed in the editor game target.
- The standalone executable passed photographic controls and persistence across
  a process restart, then a complete rendered Nominal mission.
- First rail contact: 0.3973 m/s total speed, -0.2407 m/s vertical speed,
  0.2182 degrees tilt. Both rails supported the vehicle with engines off.
- The chase camera remained stable during the 121 measured contact frames.
- Actual master-submix audio was present and contained no clipped samples.
- No fatal errors or material compilation failures occurred in these runs.
- The portable ZIP passed file-manifest SHA-256 and archive CRC verification.

The raw reports in this directory belong to the packaged executable. Source
test and stationary 1440p DLSS Quality performance evidence is under
`Docs/Validation/VisualRenewal`. Fixed-step correctness flights are not FPS
benchmarks. Broad steam shapes, distant cloud composition and terrain detail
remain art-direction work; this alpha does not claim those are finished.

Output: `Releases/Starbase-0.1.0-alpha.10/Windows/SuperHeavySim.exe`.
Portable distribution: `Releases/Starbase-0.1.0-alpha.10.zip` (1.36 GB).
Keep the extracted directory intact; the executable requires its packaged data.

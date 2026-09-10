# Alpha 9 validation — 10 September 2026

Packaged source: `1f52c6dc2b07e73ba92c05e2cdda19e4249d27d5`.
The subsequent evidence commit does not change the executable or assets.

## Performance scope

See `Clouds/measurements.json` and `../../CLOUD_RENDERING.md` for settings, raw
captures and research. The 14 retained probes cover the previous implementation,
the final cloud pipeline, ground, layer, orbital and globe views, sunset, night,
and dense overcast in native 1440p. The source-game probes use normal window
rendering with one GPU process. Their frame timers include the screenshot stall.
These are observer benchmarks, not an all-mission minimum-FPS guarantee.

## Exact packaged build

`Tools/Tests/test_packaged_alpha.ps1 -Version 0.1.0-alpha.9` passed:

- Startup revealed the scene after all 35 requested assets were ready, with no
  pending PSOs, shaders or textures. Measured readiness: 4.68 seconds on this run.
- All 69 controls checks passed.
- A complete Crosswind mission rendered 6,440 frames with the final cloud
  material and sample policy. This deterministic test uses a fixed 15 Hz render
  timestep and 120 Hz physics; it is a correctness test, not a performance result.
- Front ingress, two independent rail contacts, engine shutdown and dynamic
  unbroken tower support passed. First vertical contact speed: -0.242 m/s;
  first-contact tilt: 0.256 degrees; settled support drift: 0.0156 m.
- Chase-camera contact stability, turbulent-volume budget and audible,
  unclipped launch audio passed. Startup, controls and flight logs contain none
  of the harness's fatal/ensure/material/missing-content failure signatures.

The original machine-readable result retains `visual_review_required: true`:
the automated harness cannot assert visual quality. Manual review subsequently
inspected Loading, Home, cloud traversal at 27/35/42 seconds, orbital descent at
210 seconds, return at 317 seconds, and Secured. Selected captures and reports
are in `Packaged/`; the full moving-cloud sequence and logs remain in
`Saved/Recovery/Alpha-0.1.0-alpha.9-20260910-172529`.

The three packaged launches and sixteen post-workaround source probe
launches completed without the intermittent Nanite shader-binding startup crash.
The serial command-recording workaround is qualified by these runs, not a proof
that every possible engine crash has been eliminated.

## Visual limits retained for follow-up

The inspected globe captures no longer show the orange circular cloud boundary
or diagonal cloud-shadow-map streaks. Cloud composition follows the rocket
silhouette during layer traversal. Fine clouds are intentionally softer under
temporal reconstruction; cloud organization remains an authored weather model.
Regional ocean/terrain shading seams, a colored ground patch and repeated world
textures remain visible. Cloud-off comparisons isolate the ocean seam from the
cloud renderer. This release does not claim to fix those world-material defects.

The portable archive is accepted only after release-file SHA-256 validation,
exact inventory matching and ZIP CRC verification by `archive_alpha.py`.

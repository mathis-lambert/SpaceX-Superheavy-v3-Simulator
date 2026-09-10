# Alpha.8: final interactive recovery build

10 September 2026. This completes the interactive recovery batch documented in
[the alpha.7 implementation and source validation](ALPHA_0.1.0_ALPHA7_VALIDATION.md).
The simulation, assets and UI are unchanged from that tested implementation.
The final build retains the editor-only GameFeatureData scan correction and
changes the control audit to observe actual simulated ascent, with a five-minute
wall timeout, instead of assuming that 77 real seconds implies completed ascent.
This keeps the original launch assertion and catches genuinely stuck countdowns.

Alpha.8 uses a separate directory so the user's already-running alpha.7 is not
interrupted or replaced. Alpha.6 is also preserved. No changes are pushed.

Source validation includes 31 passing model tests, six reference returns, eight
recoverable fault replays, eight collision fixtures and two emergency descents.
The emergency replays end at 1.70–1.81 m/s while correctly reporting the tower
mission lost. Ground performance was measured with one simulator instance;
concurrent instances share the GPU and are not comparable to that benchmark.

The physical model and graphics limitations in the alpha.7 report remain in force,
including one failed engine/one jammed fin plus the shared RCS manifold, estimated
trajectory/alternate reachability, a sea-level emergency endpoint without water
physics, finite interpolated smoke caches and remaining distant-scenery detail.
The offscreen D3D12 command-list-pool failure is not qualified as fixed; normal
windowed operation is validated separately.

## Exact package validation

Pending final package checks. The manifest will identify the exact source commit.

param([int[]]$Cadences=@(60,30,15),[string]$EngineRoot='D:/Engines/UE_5.8',[string]$Prefix='Physical',[ValidateSet('Nominal','Crosswind','Offset')][string[]]$Scenarios=@('Nominal','Crosswind','Offset'),[ValidateSet(0,60,120,240,480,960)][int]$PhysicsHz=0,[switch]$JitterClock)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$saved=Join-Path $root 'Saved/Recovery'
. (Join-Path $root 'Tests/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
$results=@()
foreach ($fps in $Cadences) {
    foreach ($scenario in $Scenarios) {
        $name="${Prefix}_${scenario}_${fps}"
        $start=Get-Date
        [string[]]$clockArgs=@()
        if($PhysicsHz -gt 0){$clockArgs+="-RecoveryPhysicsHz=$PhysicsHz"}
        if($JitterClock){$clockArgs+=@('-RecoveryJitterClock')}
        & $engine (Join-Path $root 'SuperHeavySim.uproject') '/Game/Starbase/Maps/L_RecoveryLab' -game -nullrhi -unattended -UseFixedTimeStep "-FPS=$fps" @clockArgs -RecoveryAutoExit -RecoveryPhysicsAudit "-RecoveryScenario=$scenario" "-RecoveryReportName=$name" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/$name.log" *> (Join-Path $saved "$name-console.log")
        $engineExit=$LASTEXITCODE
        $file=Join-Path $saved "$name.json"
        if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $start){throw "No fresh report: $name"}
        $r=Get-Content -Raw $file | ConvertFrom-Json
        $cadenceFile=Get-Item -LiteralPath "$saved/$name-cadence.json"
        if($cadenceFile.LastWriteTime -lt $start){throw "Stale solver cadence report: $name"}
        $cadence=Get-Content -Raw -LiteralPath $cadenceFile.FullName | ConvertFrom-Json
        $inner=$cadence.inner_dynamics_steps
        $solver=$cadence.solver_steps
        $control=$cadence.game_steps
        $fixed=$cadence.configured_fixed_step_s
        $fixedClock=$cadence.async_physics -and !$cadence.substepping -and $null -ne $fixed -and $fixed -gt 0 -and [math]::Abs($solver.min_s-$fixed) -lt 1e-8 -and [math]::Abs($solver.max_s-$fixed) -lt 1e-8
        $fixedClock=$fixedClock -and $cadence.async_block_mode -eq 1 -and $cadence.configured_frame_cap_s -eq 0 -and [math]::Abs($solver.total_s-$control.total_s) -le $control.max_s+2*$fixed
        if($JitterClock){$fixedClock=$fixedClock -and $cadence.jitter_clock -and $control.max_s -ge .199 -and $control.min_s -lt .007}
        if($PhysicsHz -gt 0){$fixedClock=$fixedClock -and [math]::Abs($fixed-1.0/$PhysicsHz) -lt 1e-8}
        # Output consumers can straddle a game frame and the two-interval async
        # interpolation window. Step sizes themselves must match the fixed clock.
        $scheduled=$null -ne $inner -and $inner.count -gt 0 -and [math]::Abs($inner.total_s-$solver.total_s) -le $control.max_s+2*$fixed -and [math]::Abs($inner.max_s-$fixed) -lt 1e-8 -and [math]::Abs($inner.min_s-$fixed) -lt 1e-8
        $guidance=$cadence.flight_guidance_steps
        $guided=$null -ne $guidance -and $guidance.count -gt 0 -and [math]::Abs($guidance.max_s-$fixed) -lt 1e-8 -and [math]::Abs($guidance.min_s-$fixed) -lt 1e-8
        $upper=$cadence.upper_stage_steps
        $staged=$null -ne $upper -and $upper.count -gt 0 -and [math]::Abs($upper.min_s-$fixed) -lt 1e-8 -and [math]::Abs($upper.max_s-$fixed) -lt 1e-8 -and $r.upper_stage_delivered_impulse_ns -gt 0 -and $null -ne $r.upper_stage_propellant_balance_error_kg -and [math]::Abs($r.upper_stage_propellant_balance_error_kg) -lt .1
        $pass=$fixedClock -and $staged -and $guided -and $scheduled -and $engineExit -eq 0 -and $cadence.measured -and $r.success -and $r.physical_capture -and $r.left_rail_contact -and $r.right_rail_contact -and $r.contact_engine_shutdown -and !$r.unpowered_thrust_violation -and $r.launch_catch_axis_distance_m -lt .01
        $pass=$pass -and $r.registered_engines -eq 33 -and $r.peak_engine_force_ratio -le 1.000001 -and $r.peak_gimbal_deg -le 8.001 -and $r.upper_stage_physical -and $r.launch_hold_released
        $pass=$pass -and $r.separation_velocity_error_mps -lt .001 -and $r.separation_momentum_relative_error -lt .00001 -and $r.separation_angular_momentum_relative_error -lt .01
        $pass=$pass -and $null -ne $r.propellant_balance_error_kg -and [math]::Abs($r.propellant_balance_error_kg) -lt .1 -and $r.restraint_drift_m -lt .25 -and $r.final_speed_mps -lt .1
        $pass=$pass -and (Test-RecoveryFrontApproach -Report $r) -and $r.structural_contacts -eq 0
        $pass=$pass -and $r.tower_dynamic -and $r.tower_broken_rail_mask -eq 0 -and $r.tower_broken_hinge_mask -eq 0
        $pass=$pass -and (Test-RecoveryGentleContact -Report $r)
        $pass=$pass -and $r.solver_support_samples -gt 0 -and $r.solver_support_mask -eq 3 -and $r.left_support_impulse_ns -gt 0 -and $r.right_support_impulse_ns -gt 0
        $results+=@{scenario=$scenario;fps=$fps;physics_hz=1./$fixed;success=$pass;report="Saved/Recovery/$name.json"}
        Write-Host "$name PASS=$pass support drift=$($r.restraint_drift_m) m"
    }
}
$results | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $saved "$Prefix-matrix.json")
if($results | Where-Object {!$_.success}){throw 'Physical flight matrix contains failures'}

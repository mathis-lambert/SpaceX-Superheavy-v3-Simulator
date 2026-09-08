param([int[]]$Cadences=@(60,30,15),[string]$EngineRoot='D:/Engines/UE_5.8',[string]$Prefix='Physical',[ValidateSet('Nominal','Crosswind','Offset')][string[]]$Scenarios=@('Nominal','Crosswind','Offset'))
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$saved=Join-Path $root 'Saved/Recovery'
. (Join-Path $root 'Tools/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
$results=@()
foreach ($fps in $Cadences) {
    foreach ($scenario in $Scenarios) {
        $name="${Prefix}_${scenario}_${fps}"
        $start=Get-Date
        & $engine (Join-Path $root 'SuperHeavySim.uproject') '/Game/Starbase/Maps/L_RecoveryLab' -game -nullrhi -unattended -UseFixedTimeStep "-FPS=$fps" -RecoveryAutoExit -RecoveryPhysicsAudit "-RecoveryScenario=$scenario" "-RecoveryReportName=$name" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/$name.log" *> (Join-Path $saved "$name-console.log")
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
        # A mission-result snapshot precedes the final physics frame. The cadence
        # audit is taken after it; permit one render interval, never missing model substeps.
        $scheduled=$null -ne $inner -and $inner.count -gt $control.count -and [math]::Abs($inner.total_s-$solver.total_s) -le $control.max_s*1.01 -and [math]::Abs($inner.max_s-$solver.max_s) -lt 1e-9
        $guidance=$cadence.flight_guidance_steps
        $guided=$null -ne $guidance -and $guidance.count -gt $control.count -and [math]::Abs($guidance.max_s-$solver.max_s) -lt 1e-9
        $pass=$guided -and $scheduled -and $engineExit -eq 0 -and $cadence.measured -and $r.success -and $r.physical_capture -and $r.left_rail_contact -and $r.right_rail_contact -and $r.contact_engine_shutdown -and !$r.unpowered_thrust_violation -and $r.launch_catch_axis_distance_m -lt .01
        $pass=$pass -and $r.registered_engines -eq 33 -and $r.peak_engine_force_ratio -le 1.000001 -and $r.peak_gimbal_deg -le 8.001 -and $r.upper_stage_physical -and $r.launch_hold_released
        $pass=$pass -and $r.separation_velocity_error_mps -lt .001 -and $r.separation_momentum_relative_error -lt .00001 -and $r.separation_angular_momentum_relative_error -lt .01
        $pass=$pass -and $null -ne $r.propellant_balance_error_kg -and [math]::Abs($r.propellant_balance_error_kg) -lt .1 -and $r.restraint_drift_m -lt .25 -and $r.final_speed_mps -lt .1
        $results+=@{scenario=$scenario;fps=$fps;success=$pass;report="Saved/Recovery/$name.json"}
        Write-Host "$name PASS=$pass support drift=$($r.restraint_drift_m) m"
    }
}
$results | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $saved "$Prefix-matrix.json")
if($results | Where-Object {!$_.success}){throw 'Physical flight matrix contains failures'}

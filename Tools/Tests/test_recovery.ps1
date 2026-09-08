param(
    [string]$EngineRoot = 'D:/Engines/UE_5.8',
    [string[]]$Scenarios = @('Nominal','Crosswind','Offset'),
    [int[]]$FrameRates = @(60)
)
$ErrorActionPreference = 'Stop'
$projectDir = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $projectDir 'SuperHeavySim.uproject'
$exe = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$reportDir = Join-Path $projectDir ('Saved/Recovery/Tests/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
$reports = @()
foreach ($scenario in $Scenarios) {
    if ($scenario -notin @('Nominal','Crosswind','Offset')) { throw "Unknown scenario: $scenario" }
    foreach ($fps in $FrameRates) {
        if ($fps -lt 2 -or $fps -gt 240) { throw 'Frame rate must be between 2 and 240.' }
        $caseName = "$scenario-$fps"
        $logPath = Join-Path $reportDir "$caseName.log"
        $started = Get-Date
        & $exe $project '/Game/Starbase/Maps/L_RecoveryLab' -game -nullrhi -unattended -nosplash -nosound -DisablePython -SCCProvider=None -NoLoadingScreen -UseFixedTimeStep "-FPS=$fps" -RecoveryAutoExit "-RecoveryScenario=$scenario" "-abslog=$logPath"
        # Some editor launchers return zero even when the game requests an error exit.
        # The fresh, machine-readable flight result is the acceptance authority.
        $resultPath = Join-Path $projectDir "Saved/Recovery/$scenario.json"
        if (!(Test-Path $resultPath) -or (Get-Item $resultPath).LastWriteTime -lt $started) { throw "$caseName produced no fresh result. See $logPath" }
        $result = Get-Content $resultPath -Raw | ConvertFrom-Json
        $passed = $result.success -and $result.capture_error_m -le 1.5 -and $result.capture_lug_error_m -le 0.35 -and $result.capture_heading_error_deg -lt 2 -and $result.capture_speed_mps -lt 0.6 -and $result.capture_tilt_deg -lt 1.5 -and $result.restraint_drift_m -le 0.25 -and $result.final_speed_mps -lt 0.1
        $passed = $passed -and $result.peak_altitude_m -gt 80000 -and $result.peak_altitude_m -lt 130000 -and $result.boostback_ignition_altitude_m -gt 50000 -and $result.peak_downrange_m -gt 20000
        $passed = $passed -and $result.unpowered_seconds -gt 60 -and !$result.unpowered_thrust_violation -and $result.fin_control_seconds -gt 5 -and $result.landing_ignition_altitude_m -gt 500 -and $result.landing_ignition_altitude_m -lt 5000
        $passed = $passed -and $result.mass_kg -lt $result.launch_mass_kg*0.2 -and $result.propellant_remaining_kg -gt 0 -and $null -ne $result.propellant_balance_error_kg -and [math]::Abs($result.propellant_balance_error_kg) -lt 0.1
        $result | Add-Member NoteProperty frame_rate $fps
        $result | Add-Member NoteProperty acceptance_passed $passed
        $result | ConvertTo-Json | Set-Content (Join-Path $reportDir "$caseName.json") -Encoding UTF8
        Copy-Item -LiteralPath (Join-Path $projectDir "Saved/Recovery/$scenario.csv") -Destination (Join-Path $reportDir "$caseName.csv")
        $reports += $result
        Write-Host "$caseName : passed=$passed capture=$([math]::Round($result.capture_error_m,3)) m duration=$([math]::Round($result.duration_s,1)) s"
    }
}
$reports | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $reportDir 'summary.json') -Encoding UTF8
Write-Host "Reports: $reportDir"
if ($reports.Where({!$_.acceptance_passed}).Count -gt 0) { throw 'Recovery acceptance failed. Inspect summary.json and flight traces.' }

param([string]$EngineRoot='D:/Engines/UE_5.8',[ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Prefix='Contact')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$saved=Join-Path $root 'Saved/Recovery'
. (Join-Path $root 'Tests/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
foreach($fixture in @('EmptyTower','OpenTower','SleepingClose','Centered','AlongRail','WrongHeading','SideImpact','Overload')){
    $name="$Prefix$fixture"
    $started=Get-Date
    & $engine (Join-Path $root 'SuperHeavySim.uproject') /Game/Starbase/Maps/L_RecoveryLab -game -nullrhi -unattended -UseFixedTimeStep -FPS=60 -RecoveryAutoExit "-RecoveryContactFixture=$fixture" "-RecoveryReportName=$name" -DisablePython -nosplash -SCCProvider=None "-abslog=$saved/$name.log" *> "$saved/$name-console.log"
    $engineExit=$LASTEXITCODE
    $file=Join-Path $saved "$name.json"
    if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $started){throw "No fresh fixture report: $fixture"}
    $report=Get-Content -Raw $file|ConvertFrom-Json
    if($engineExit -ne 0 -or !$report.success -or $report.solver_support_samples -le 0){throw "Contact fixture failed: $fixture"}
    if($fixture -in @('Centered','AlongRail')){
        # A released, initially stationary body settles without propulsion.
        # Its total upward reaction must balance gravity's integrated impulse.
        $weightImpulse=$report.mass_kg*9.80665*$report.dynamics_time_s
        if($report.solver_support_mask -ne 3 -or [math]::Abs($report.support_vertical_impulse_ns/$weightImpulse-1) -gt .02){throw 'Rail reaction does not balance the unpowered body weight impulse'}
    }
    Write-Host "$name PASS"
}

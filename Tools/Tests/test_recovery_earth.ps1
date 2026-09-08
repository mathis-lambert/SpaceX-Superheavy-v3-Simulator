param([string]$EngineRoot='D:/Engines/UE_5.8')
$ErrorActionPreference='Stop'
$projectDir=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project=Join-Path $projectDir 'SuperHeavySim.uproject'
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$saved=Join-Path $projectDir 'Saved/Recovery'
$settings=Join-Path $projectDir 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$previousSettings=if(Test-Path -LiteralPath $settings){[IO.File]::ReadAllText($settings)}else{$null}
try {
    $started=Get-Date
    & $engine $project '/Game/Starbase/Maps/L_RecoveryLab' -game -windowed -ResX=1920 -ResY=1080 -RecoveryEarthAudit -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/EarthAudit.log" *> (Join-Path $saved 'earth-audit-console.log')
    $auditFile=Join-Path $saved 'EarthAudit/result.json'
    if(!(Test-Path $auditFile) -or (Get-Item $auditFile).LastWriteTime -lt $started){throw 'No fresh camera audit report'}
    $audit=Get-Content -Raw $auditFile | ConvertFrom-Json
    if(!$audit.success){throw 'Earth camera audit failed'}
    Write-Host 'Live camera selection, distant views, transition and pause: PASS'
    $started=Get-Date
    & $engine $project '/Game/Starbase/Maps/L_RecoveryLab' -game -windowed -ResX=1920 -ResY=1080 -UseFixedTimeStep -FPS=30 -RecoveryReview -RecoveryEarthReview -RecoveryAutoExit -RecoveryReportName=EarthFlightFinal -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/earth-flight-final.log" *> (Join-Path $saved 'earth-flight-final-console.log')
    $flightFile=Join-Path $saved 'EarthFlightFinal.json'
    if(!(Test-Path $flightFile) -or (Get-Item $flightFile).LastWriteTime -lt $started){throw 'No fresh flight report'}
    $flight=Get-Content -Raw $flightFile | ConvertFrom-Json
    if(!$flight.success -or $flight.unpowered_thrust_violation -or $flight.capture_lug_error_m -gt .35){throw 'Earth flight acceptance failed'}
    Write-Host ('Full flight: PASS; apogee {0:N2} km; catch {1:N3} m/s; fitting error {2:N3} m' -f ($flight.peak_altitude_m/1000),$flight.capture_speed_mps,$flight.capture_lug_error_m)
} finally {
    if($null -ne $previousSettings){[IO.File]::WriteAllText($settings,$previousSettings)}
}

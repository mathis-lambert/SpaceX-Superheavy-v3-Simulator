param([string]$EngineRoot='D:/Engines/UE_5.8')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$saved=Join-Path $root 'Saved/Recovery'
foreach($fixture in @('Centered','WrongHeading','SideImpact')){
    $started=Get-Date
    & $engine (Join-Path $root 'SuperHeavySim.uproject') /Game/Starbase/Maps/L_RecoveryLab -game -nullrhi -unattended -UseFixedTimeStep -FPS=60 -RecoveryAutoExit "-RecoveryContactFixture=$fixture" "-RecoveryReportName=Contact$fixture" -DisablePython -nosplash -SCCProvider=None "-abslog=$saved/Contact$fixture.log" *> "$saved/Contact$fixture-console.log"
    $file=Join-Path $saved "Contact$fixture.json"
    if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $started){throw "No fresh fixture report: $fixture"}
    if(!(Get-Content -Raw $file|ConvertFrom-Json).success){throw "Contact fixture failed: $fixture"}
    Write-Host "Contact$fixture PASS"
}

param([string]$EngineRoot='D:/Engines/UE_5.8',[int[]]$Cadences=@(30,60))
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
foreach($hz in $Cadences){foreach($fixture in @('WaterVertical','WaterHorizontal','WaterFast')){
    $name="Marine-$fixture-$hz";$started=Get-Date
    & $engine "$root/SuperHeavySim.uproject" /Game/Starbase/Maps/L_RecoveryLab -game -nullrhi -unattended -UseFixedTimeStep "-FPS=$hz" -RecoveryAutoExit "-RecoveryContactFixture=$fixture" "-RecoveryReportName=$name" -DisablePython -nosplash -SCCProvider=None "-abslog=$root/Saved/Recovery/$name.log" *> "$root/Saved/Recovery/$name-console.log"
    if($LASTEXITCODE -ne 0){throw "$name process failed"}
    $path="$root/Saved/Recovery/$name.json"
    if(!(Test-Path $path) -or (Get-Item $path).LastWriteTime -lt $started){throw "$name report missing"}
    $report=Get-Content $path -Raw | ConvertFrom-Json
    if(!$report.success -or !$report.water_contact -or $report.submerged_volume_m3 -le 0 -or $report.final_speed_mps -gt .1){throw "$name flotation failed"}
    if([math]::Abs($report.submerged_volume_m3*1025/$report.mass_kg-1) -gt .03){throw "$name displacement does not balance weight"}
    Write-Host "$name PASS"
}}

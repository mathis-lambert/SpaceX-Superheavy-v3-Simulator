param([int[]]$Cadences=@(60,15),[string]$Prefix='Resilience',[string]$EngineRoot='D:/Engines/UE_5.8',[string[]]$SelectedCases=@('RcsPulse','RcsExtended','FinJam','Combined'))
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$saved=Join-Path $root 'Saved/Recovery'
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$cases=@(
    @{Name='RcsPulse';Faults='3:0:193:1;3:0:219:1;3:0:271:1';RcsSeconds=3},
    @{Name='RcsExtended';Faults='3:0:193:12';RcsSeconds=12},
    @{Name='FinJam';Faults='2:0:236:200';RcsSeconds=0},
    @{Name='Combined';Faults='1:8:25:600;2:0:236:178;3:0:193:3;3:0:219:11.3;3:0:271:7;3:0:288:3.5';RcsSeconds=24.8}
)
$results=@()
if(@($cases | Where-Object { $_.Name -in $SelectedCases }).Count -ne $SelectedCases.Count){throw 'Unknown or repeated resilience case'}
foreach($hz in $Cadences){foreach($case in $cases | Where-Object { $_.Name -in $SelectedCases }){
    $name="${Prefix}_$($case.Name)_$hz";$started=Get-Date
    & $engine "$root/SuperHeavySim.uproject" /Game/Starbase/Maps/L_RecoveryLab -game -nullrhi -unattended -UseFixedTimeStep "-FPS=$hz" -RecoveryAutoExit -RecoveryPhysicsAudit "-RecoveryFaults=$($case.Faults)" "-RecoveryReportName=$name" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/$name.log" *> "$saved/$name-console.log"
    $exitCode=$LASTEXITCODE;$path=Join-Path $saved "$name.json"
    if(!(Test-Path $path) -or (Get-Item $path).LastWriteTime -lt $started){throw "Missing fresh report $name"}
    $r=Get-Content -Raw $path | ConvertFrom-Json
    $timing=[Math]::Abs($r.rcs_disabled_s-$case.RcsSeconds) -lt .025
    $results+=@{name=$name;exit_code=$exitCode;catch=$r.success;alternate=$r.alternate_recovery;reason=$r.reason;outage_timing=$timing;rcs_disabled_s=$r.rcs_disabled_s;corrective_burn_s=$r.corrective_burn_s}
    $results | ConvertTo-Json -Depth 5 | Set-Content "$saved/$Prefix-matrix.json"
    Write-Host "$name catch=$($r.success) alternate=$($r.alternate_recovery) timing=$timing"
}}
if(@($results | Where-Object { !$_.outage_timing -or $_.exit_code -ne 0 -or !$_.catch }).Count){throw 'A recoverable fault replay failed physical capture or its scheduled duration; inspect reports.'}

param([int[]]$Cadences=@(60,15),[string]$Prefix='Emergency',[string]$EngineRoot='D:/Engines/UE_5.8')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$saved=Join-Path $root 'Saved/Recovery'
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
. (Join-Path $root 'Tests/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
$results=@()
foreach($hz in $Cadences){
    $name="${Prefix}_SeparationRcs_$hz";$started=Get-Date
    & $engine "$root/SuperHeavySim.uproject" /Game/Starbase/Maps/L_RecoveryLab -game -nullrhi -unattended -UseFixedTimeStep "-FPS=$hz" -RecoveryAutoExit -RecoveryPhysicsAudit '-RecoveryFaults=3:0:130:65' "-RecoveryReportName=$name" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/$name.log" *> "$saved/$name-console.log"
    $exitCode=$LASTEXITCODE;$path=Join-Path $saved "$name.json"
    if(!(Test-Path -LiteralPath $path) -or (Get-Item $path).LastWriteTime -lt $started){throw "No fresh emergency report: $name"}
    $r=Get-Content -Raw -LiteralPath $path|ConvertFrom-Json
    $pass=$exitCode -eq 0 -and !$r.success -and $r.alternate_recovery -and
        $r.reason -eq 'Emergency surface contact / tower mission not recovered' -and
        [Math]::Abs($r.rcs_disabled_s-65) -lt .025 -and
        $r.final_speed_mps -lt 3 -and [Math]::Abs($r.final_altitude_m) -lt 3 -and $r.landing_burn_seconds -gt 5
    $results+=@{name=$name;pass=$pass;mission_success=$r.success;reason=$r.reason;speed_mps=$r.final_speed_mps;burn_s=$r.landing_burn_seconds;outage_s=$r.rcs_disabled_s}
    $results|ConvertTo-Json -Depth 5|Set-Content -Encoding utf8 -LiteralPath "$saved/$Prefix-matrix.json"
    Write-Host "$name emergency=$pass contact=$($r.final_speed_mps) m/s"
}
if(@($results|Where-Object {!$_.pass}).Count){throw 'Emergency recovery did not meet the measured impact-mitigation contract'}

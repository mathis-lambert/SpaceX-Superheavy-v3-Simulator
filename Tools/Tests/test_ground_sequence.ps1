param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [switch]$HotAbort,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Prefix='GroundSequence',
    [ValidateRange(0,24)][double]$Hour=14
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$saved=Join-Path $root 'Saved/Recovery'
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$prefs=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$before=if(Test-Path -LiteralPath $prefs){[IO.File]::ReadAllBytes($prefs)}else{$null}
. (Join-Path $root 'Tools/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
$source=Join-Path $saved $(if($HotAbort){'GroundAbortAudit'}else{'GroundAudit'})
[string[]]$extra=@()
if($HotAbort){$extra+='-RecoveryGroundHotAbort'}
$started=Get-Date
try {
    & $engine "$root/SuperHeavySim.uproject" /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ForceRes -ResX=1920 -ResY=1080 -RecoveryGroundAudit @extra "-RecoveryHour=$($Hour.ToString([Globalization.CultureInfo]::InvariantCulture))" -RecoveryReconstruction=3 -unattended -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/$Prefix.log" *> "$saved/$Prefix-console.log"
    if($LASTEXITCODE -ne 0){throw "Ground sequence exited with $LASTEXITCODE"}
    $file=Get-Item -LiteralPath "$source/result.json"
    if($file.LastWriteTime -lt $started){throw 'Ground audit did not produce a fresh report'}
    $report=Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json
    if(!$report.success){throw 'Ground sequence checks failed'}
    $required=@('Home.png','TerminalCount.png','Hold.png','Deluge.png','Ignition.png')+$(if($HotAbort){'SafeShutdown.png'}else{'Liftoff.png'})
    foreach($name in $required){
        $shot=Get-Item -LiteralPath (Join-Path $source $name)
        if($shot.LastWriteTime -lt $started){throw "Stale screenshot: $name"}
    }
    if(Select-String -Quiet -LiteralPath "$saved/$Prefix.log" -Pattern 'Failed to compile Material|Fatal error:'){throw 'Ground audit contains a rendering failure'}
    $destination=Join-Path $saved $Prefix
    $null=New-Item -ItemType Directory -Force -Path $destination
    foreach($name in ($required+@('result.json'))){Copy-Item -LiteralPath (Join-Path $source $name) -Destination (Join-Path $destination $name)}
    Write-Host "$Prefix PASS ($($report.checks.Count) checks). Images require visual inspection: $destination"
} finally {
    if($null -ne $before){[IO.File]::WriteAllBytes($prefs,$before)}
    elseif(Test-Path -LiteralPath $prefs){Remove-Item -LiteralPath $prefs}
}

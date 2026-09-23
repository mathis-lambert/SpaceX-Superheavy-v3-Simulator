param([string]$EngineRoot='D:/Engines/UE_5.8')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$saved=Join-Path $root 'Saved/Recovery'
$started=Get-Date
$parameters=@('-game','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-RecoveryStartupAudit','-RecoveryReconstruction=3','-nosplash','-DisablePython','-SCCProvider=None',"-abslog=$saved/startup-audit.log")
$Executable=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$parameters=@("$root/SuperHeavySim.uproject",'/Game/Starbase/Maps/L_RecoveryLab')+$parameters
& $Executable @parameters *> "$saved/startup-audit-console.log"
if($LASTEXITCODE -ne 0){throw 'Startup process failed'}
$report=Get-Item -LiteralPath "$saved/startup-report.json"
if($report.LastWriteTime -lt $started){throw 'No fresh startup report'}
$r=Get-Content -Raw -LiteralPath $report.FullName | ConvertFrom-Json
if(!$r.ready -or $r.failed -or $r.assets_loaded -ne $r.assets_requested -or $r.pending_psos_at_reveal -ne 0 -or $r.pending_textures_at_reveal -ne 0 -or $r.pending_shaders_at_reveal -ne 0){throw 'Scene revealed before resources were ready'}
foreach($name in 'Loading','Home'){
    $shot=Get-Item -LiteralPath "$saved/Startup/$name.png"
    if($shot.LastWriteTime -lt $started){throw "Missing fresh $name capture"}
}
Write-Host "Startup PASS: $($r.assets_loaded) assets, $([math]::Round($r.engine_elapsed_seconds,2)) s engine startup. Inspect Loading.png and Home.png."

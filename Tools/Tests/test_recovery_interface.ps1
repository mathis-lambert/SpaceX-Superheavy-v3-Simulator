param([string]$EngineRoot = 'D:/Engines/UE_5.8')
$ErrorActionPreference = 'Stop'
$projectDir = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $projectDir 'SuperHeavySim.uproject'
$resultPath = Join-Path $projectDir 'Saved/Recovery/InterfaceAudit/result.json'
$logPath = Join-Path $projectDir 'Saved/Recovery/InterfaceAudit.log'
$started = Get-Date
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $project '/Game/Starbase/Maps/L_RecoveryLab' -game -RecoveryUIAudit -nosplash -DisablePython -SCCProvider=None "-abslog=$logPath"
if (!(Test-Path $resultPath) -or (Get-Item $resultPath).LastWriteTime -lt $started) { throw "No fresh interface audit result. See $logPath" }
$result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
$result.checks | ForEach-Object { Write-Host $_ }
if (!$result.success) { throw "Interface acceptance failed. See $resultPath" }
Write-Host "Interface, world pause, display rollback and engine lights validated. Screenshots: $(Split-Path $resultPath)"

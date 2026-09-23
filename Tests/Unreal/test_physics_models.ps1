param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Prefix='PhysicsModels'
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$saved=Join-Path $root 'Saved/Recovery'
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$export=Join-Path $saved "$Prefix-Unit"
$null=New-Item -ItemType Directory -Force -Path $saved
. (Join-Path $root 'Tests/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
$started=Get-Date
& $engine "$root/SuperHeavySim.uproject" -nullrhi -unattended -nosplash -DisablePython -SCCProvider=None '-ExecCmds=Automation RunTests Recovery.' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$export" "-abslog=$saved/$Prefix-unit.log" *> "$saved/$Prefix-unit-console.log"
$engineExit=$LASTEXITCODE
$file=Get-Item -LiteralPath "$export/index.json"
if($file.LastWriteTime -lt $started){throw 'Unreal produced no fresh automation report'}
$report=Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json
$failures=@($report.tests | Where-Object { $_.state -ne 'Success' -or $_.errors -gt 0 })
# Unreal's TestExit may return zero even with failing automation assertions.
# Require the actual exported test results, including discovery and execution.
if($engineExit -ne 0 -or $report.tests.Count -eq 0 -or $report.failed -gt 0 -or $report.notRun -gt 0 -or $failures.Count -gt 0){
    $names=($failures | ForEach-Object fullTestPath) -join ', '
    throw "Physics model automation failed: $names. Inspect $export/index.json"
}
Write-Host "$Prefix PASS ($($report.tests.Count) tests, $($report.succeededWithWarnings) with warnings)"

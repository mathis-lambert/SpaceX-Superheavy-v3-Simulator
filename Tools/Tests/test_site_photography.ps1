param([string]$EngineRoot='D:/Engines/UE_5.8',[string]$GameExecutable='')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$run=Join-Path $root ('Saved/Recovery/Photography-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
$null=New-Item -ItemType Directory -Force -Path $run
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if(!$GameExecutable){
    & $engine "$root/SuperHeavySim.uproject" -nullrhi -unattended -nosplash -DisablePython -SCCProvider=None '-ExecCmds=Automation RunTests Recovery.Presentation.SolarGeography' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$run/Solar" "-abslog=$run/Solar.log" *> "$run/Solar-console.log"
    $unit=Get-Content -Raw -LiteralPath "$run/Solar/index.json" | ConvertFrom-Json
    if($unit.failed -gt 0 -or $unit.succeeded -ne 1){throw 'Solar geography assertions failed'}
}
$userDirectory="$run/User/"
foreach($pass in @('Initial','Reload')){
    $reload=if($pass -eq 'Reload'){'-RecoveryPhotoReload'}else{'-RecoveryPhotoInitial'}
    $arguments=@('/Game/Starbase/Maps/L_RecoveryLab','-game','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-RecoveryPhotoAudit',$reload,'-RecoveryReconstruction=3','-nosplash','-DisablePython','-SCCProvider=None',"-UserDir=$userDirectory","-abslog=$run/$pass.log")
    $started=Get-Date
    if(!$GameExecutable){& $engine "$root/SuperHeavySim.uproject" @arguments *> "$run/$pass-console.log"; $code=$LASTEXITCODE}
    else {
        $quoted=@($arguments | ForEach-Object {'"'+$_.Replace('"','\"')+'"'})
        $process=Start-Process -FilePath $GameExecutable -ArgumentList $quoted -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput "$run/$pass-console.log" -RedirectStandardError "$run/$pass-stderr.log"
        $code=$process.ExitCode
    }
    if($code -ne 0){throw "Rendered $pass audit exited with $code"}
    $result=Get-ChildItem -LiteralPath "$run/User" -Recurse -Filter result.json | Where-Object { $_.Directory.Name -eq 'Photography' } | Select-Object -First 1
    if(!$result -or $result.LastWriteTime -lt $started){throw 'No fresh photographic audit report'}
    Copy-Item -LiteralPath $result.FullName -Destination "$run/$pass-result.json"
    $report=Get-Content -Raw -LiteralPath $result.FullName | ConvertFrom-Json
    if(!$report.success){throw "Photographic $pass assertions failed"}
    if(Select-String -Quiet -LiteralPath "$run/$pass.log" -Pattern 'Failed to compile Material|Fatal error:'){throw 'Rendered material failure'}
}
Write-Host "PHOTOGRAPHY_PASS $run"

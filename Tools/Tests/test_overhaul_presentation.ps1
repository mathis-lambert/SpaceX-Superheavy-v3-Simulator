$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine='D:/Engines/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project=Join-Path $root 'SuperHeavySim.uproject'
$saved=Join-Path $root 'Saved/Recovery'
$settings=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$previous=[IO.File]::ReadAllText($settings)
try {
    foreach($audit in @(@{Flag='RecoveryOverhaulAudit';Report='Overhaul/result.json'},@{Flag='RecoveryEarthAudit';Report='EarthAudit/result.json'})) {
        $start=Get-Date
        & $engine $project /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ResX=1920 -ResY=1080 "-$($audit.Flag)" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/$($audit.Flag)-final.log" *> "$saved/$($audit.Flag)-final-console.log"
        $file=Join-Path $saved $audit.Report
        if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $start){throw "Missing fresh report: $file"}
        if(!(Get-Content -Raw $file|ConvertFrom-Json).success){throw "Audit failed: $file"}
        Write-Host "$($audit.Flag) PASS"
    }
    $start=Get-Date
    & $engine $project /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ResX=1920 -ResY=1080 -UseFixedTimeStep -FPS=15 -RecoveryReview -RecoveryEarthReview -RecoveryAutoExit -RecoveryScenario=Crosswind -RecoveryReportName=OverhaulRenderedFinal15 -RecoveryHour=17.9 -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/overhaul-rendered-final15.log" *> "$saved/overhaul-rendered-final15-console.log"
    $file=Join-Path $saved 'OverhaulRenderedFinal15.json'
    if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $start){throw 'No fresh rendered flight report'}
    $r=Get-Content -Raw $file|ConvertFrom-Json
    if(!$r.success -or !$r.left_rail_contact -or !$r.right_rail_contact -or !$r.contact_engine_shutdown){throw 'Rendered physical capture failed'}
    Write-Host "OverhaulRenderedFinal15 PASS drift=$($r.restraint_drift_m) m"
} finally {[IO.File]::WriteAllText($settings,$previous)}

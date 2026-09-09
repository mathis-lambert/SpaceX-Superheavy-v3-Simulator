param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [string]$Executable='',
    [ValidateSet(0,1,2,3,4)][int[]]$Modes=@(0,3),
    [ValidateSet('Home','Launch','Flight')][string[]]$Scenes=@('Home','Launch'),
    [string]$Prefix='Strict',
    [ValidateSet(0,1)][int]$HardwareRayTracing=0,
    [ValidateRange(0,24)][double]$Hour=17.9,
    [ValidateSet(1080,1440,2160)][int[]]$Heights=@(1440),
    [ValidateSet(0,1,3)][int[]]$CloudModes=@(3),
    [ValidateSet(30,60)][int]$SimulationHz=60
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$runtimeSaved=Join-Path $root 'Saved'
$runtimeArguments=@("$root/SuperHeavySim.uproject",'/Game/Starbase/Maps/L_RecoveryLab','-game')
if($Executable){
    $engine=(Resolve-Path -LiteralPath $Executable).Path
    $userDirectory=Join-Path $root "Saved/Recovery/Benchmarks/$Prefix/User"
    $runtimeSaved=Join-Path $userDirectory 'Saved'
    $runtimeArguments=@("-UserDir=$userDirectory/")
}
$prefs=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$before=if(Test-Path -LiteralPath $prefs){[IO.File]::ReadAllBytes($prefs)}else{$null}
$saved=Join-Path $root 'Saved/Recovery'
$null=New-Item -ItemType Directory -Force -Path $saved
. (Join-Path $root 'Tools/Shared/validation_evidence.ps1')
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
try {
  foreach($mode in $Modes){
    $label=@('Native','TSRQuality','DLAA','DLSSQuality','DLSSBalanced')[$mode]
    foreach($height in $Heights){
    $width=[int]($height*16/9)
    foreach($cloud in $CloudModes){
    # Launch now includes the full 60-second terminal count plus 40 s of ascent.
    # Full-flight coverage includes capture; actual phases are recorded in CSV.
    foreach($case in @(@{Name='Home';Seconds=20;Args=@()},@{Name='Launch';Seconds=100;Args=@('-RecoveryNoMenu')},@{Name='Flight';Seconds=540;Args=@('-RecoveryNoMenu','-RecoveryEndProfileOnResult')})){
        if($case.Name -notin $Scenes){continue}
        $started=Get-Date
        $extra=$case.Args
        $frames=$case.Seconds*$SimulationHz
        $name="$Prefix$($case.Name)${label}${height}Cloud${cloud}Hz${SimulationHz}"
        $metadata=@{schema_version=2;scene=$case.Name;output_width=$width;output_height=$height;reconstruction=$mode;cloud_mode=$cloud;hardware_ray_tracing=$HardwareRayTracing;solar_hour=$Hour;simulation_hz=$SimulationHz;requested_frames=$frames;utc_started=$started.ToUniversalTime().ToString('o');executable=$engine;packaged=[bool]$Executable}
        $metadata | ConvertTo-Json | Set-Content -Encoding utf8 -LiteralPath "$saved/$name.capture.json"
        $arguments=$runtimeArguments+@('-windowed','-ForceRes',"-ResX=$width","-ResY=$height",'-DisablePython','-nosplash','-unattended','-RecoveryPerformanceAudit','-RecoveryReview',"-RecoveryReviewName=$name",'-RecoveryScenario=Nominal','-UseFixedTimeStep',"-FPS=$SimulationHz",'-csvGpuStats','-ExitAfterCsvProfiling')+$extra+@("-RecoveryHour=$($Hour.ToString([Globalization.CultureInfo]::InvariantCulture))","-RecoveryRayTracing=$HardwareRayTracing","-RecoveryReconstruction=$mode","-ExecCmds=r.SetRes ${width}x${height}w,t.MaxFPS 0,r.VSync 0,r.VolumetricRenderTarget.Mode $cloud,csvprofile frames=$frames","-abslog=$saved/$name.log")
        $quoted=@($arguments | ForEach-Object {'"'+[regex]::Replace([regex]::Replace($_,'(\\*)"','$1$1\"'),'(\\+)$','$1$1')+'"'})
        $process=Start-Process -FilePath $engine -WorkingDirectory (Split-Path -Parent $engine) -ArgumentList ($quoted -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$saved/$name-console.log" -RedirectStandardError "$saved/$name-stderr.log"
        if($process.ExitCode -ne 0){throw "Benchmark failed: $($case.Name)"}
        if(Select-String -Quiet -Path "$root/Saved/Recovery/$name.log" -Pattern 'Failed to compile Material|Fatal error:'){throw 'Invalid rendering in benchmark'}
        if(!(Select-String -Quiet -Path "$root/Saved/Recovery/$name.log" -Pattern "RECOVERY_RECONSTRUCTION mode=$mode ")){throw "Requested reconstruction unavailable: $label"}
        $csv=Get-ChildItem "$runtimeSaved/Profiling/CSV" -Filter '*.csv' | Where-Object LastWriteTime -ge $started | Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if(!$csv){throw "No fresh CSV: $($case.Name)"}
        $columns=(Get-Content -LiteralPath $csv.FullName -TotalCount 1).Split(',')
        if('GPU/VolumetricCloud' -notin $columns){throw "Cloud GPU pass missing; this capture cannot measure cloud performance: $name"}
        $destination="$root/Saved/Recovery/$name.csv"
        Copy-Item -LiteralPath $csv.FullName -Destination $destination
        Write-Host $destination
    }
    }
    }
  }
} finally {
    if($null -ne $before){[IO.File]::WriteAllBytes($prefs,$before)}
    elseif(Test-Path -LiteralPath $prefs){Remove-Item -LiteralPath $prefs}
}

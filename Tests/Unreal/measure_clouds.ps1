param(
    [string]$Prefix='Clouds',
    [ValidateSet(-1,0,1,2,3)][int]$CloudMode=-1,
    [int]$Upsampling=3,
    [switch]$NoClouds,
    [switch]$NoCloudShadows,
    [int]$Reconstruction=3,
    [string[]]$Views=@('Ground','Layer','Above','Orbit','Globe'),
    [double]$Hour=12,
    [int]$Weather=2
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine='D:/Engines/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$saved=Join-Path $root 'Saved/Recovery'
$preferences=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$previous=if(Test-Path -LiteralPath $preferences){[IO.File]::ReadAllBytes($preferences)}else{$null}
$cases=@{Ground=@(100,20);Layer=@(1600,0);CloudTop=@(2600,-5);Above=@(14000,-8);Orbit=@(94000,-55);Globe=@(9000000,-90)}
try {
foreach($view in $Views){
    if(Get-Process -Name SuperHeavySim,UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Close other Unreal/game instances before measuring GPU performance'}
    if(!$cases.ContainsKey($view)){throw "Unknown view $view"}
    $name="$Prefix-$view"
    $started=Get-Date
    $height,$pitch=$cases[$view]
    $args=@("$root/SuperHeavySim.uproject",'/Game/Starbase/Maps/L_RecoveryLab','-game','-windowed','-ForceRes','-ResX=2560','-ResY=1440','-nosplash','-unattended','-DisablePython','-RecoveryInteractiveAudit','-RecoveryCloudBenchmark','-csvGpuStats','-ExitAfterCsvProfiling',"-CloudHeight=$height","-CloudPitch=$pitch","-CloudWeather=$Weather","-RecoveryHour=$Hour","-RecoveryReconstruction=$Reconstruction",'-RecoveryRayTracing=0',"-RecoveryReviewName=$name","-abslog=$saved/$name.log","-ExecCmds=t.MaxFPS 0,r.VSync 0")
    if($CloudMode -ge 0){$args[-1]+=",r.VolumetricRenderTarget.Mode $CloudMode"}
    $args[-1]+=",r.VolumetricRenderTarget.UpsamplingMode $Upsampling"
    if($NoClouds){$args[-1]+=",r.VolumetricCloud 0"}
    if($NoCloudShadows){$args[-1]+=",r.VolumetricCloud.ShadowMap 0"}
    $quoted=@($args|ForEach-Object{'"'+$_+'"'})
    $p=Start-Process $engine -ArgumentList ($quoted -join ' ') -WorkingDirectory $root -WindowStyle Hidden -Wait -PassThru
    if($p.ExitCode -ne 0){throw "Cloud probe failed $name"}
    # Editor -game loads editor GameFeatures; the shipping target excludes it.
    # Retain this known configuration diagnostic in the log, reject render errors.
    if(Select-String -Quiet -LiteralPath "$saved/$name.log" -Pattern 'Fatal error:|Failed to compile Material|LogD3D12RHI: Error|LogRenderer: Error'){throw "Invalid render $name"}
    $modeMatch=Select-String -LiteralPath "$saved/$name.log" -Pattern 'CLOUD_PROBE_MODE ([0-3])' | Select-Object -Last 1
    if(!$modeMatch){throw "Missing measured cloud mode $name"}
    $actualMode=[int]$modeMatch.Matches[0].Groups[1].Value
    $expectedMode=if($CloudMode -ge 0){$CloudMode}elseif($height -gt 12000){1}else{0}
    if($actualMode -ne $expectedMode){throw "Unexpected cloud mode $actualMode for $name"}
    $csv=Get-ChildItem "$root/Saved/Profiling/CSV" -Filter '*.csv' | Where-Object LastWriteTime -ge $started | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if(!$csv){throw "Missing GPU capture $name"}
    Copy-Item -LiteralPath $csv.FullName -Destination "$saved/$name.csv"
    @{view=$view;camera_altitude_m=$height;pitch_deg=$pitch;hour=$Hour;weather=$Weather;requested_cloud_mode=$CloudMode;actual_cloud_mode=$actualMode;runtime_cloud_policy='Adaptive: mode 1 above 12 km, mode 0 below 10.5 km';upsampling=$Upsampling;clouds_enabled=(!$NoClouds);cloud_shadows=(!$NoCloudShadows);reconstruction=$Reconstruction;output=@(2560,1440);warmup_seconds=15;capture_frames=360} | ConvertTo-Json | Set-Content -LiteralPath "$saved/$name.capture.json"
    Write-Host "Captured $name"
}
} finally {
    if($null -ne $previous){[IO.File]::WriteAllBytes($preferences,$previous)}
    elseif(Test-Path -LiteralPath $preferences){Remove-Item -LiteralPath $preferences}
}

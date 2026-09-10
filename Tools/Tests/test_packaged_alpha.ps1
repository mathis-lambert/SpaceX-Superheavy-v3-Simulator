param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$')][string]$Version='0.1.0-alpha.7'
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$archive=Join-Path $root "Releases/Starbase-$Version"
$exe=Join-Path $archive 'Windows/SuperHeavySim.exe'
$manifest=Get-Content -Raw -LiteralPath "$archive/build-manifest.json" | ConvertFrom-Json
foreach($entry in $manifest.files){
    $path=Join-Path $archive $entry.path
    if(!(Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256){throw "Package checksum mismatch: $($entry.path)"}
}
. (Join-Path $root 'Tools/Shared/validation_evidence.ps1')
$audit=Join-Path $root "Saved/Recovery/Alpha-$Version-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
$userDir=Join-Path $audit 'User'
$saved=Join-Path $userDir 'Saved/Recovery'
$null=New-Item -ItemType Directory -Force -Path $audit
function Read-FreshAlphaReport([string]$Name,[datetime]$Started){
    $path=Join-Path $saved $Name
    if(!(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTime -lt $Started){throw "Missing fresh packaged report: $Name"}
    $report=Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
    if(!$report.success){throw "Packaged audit failed: $Name"}
    return $report
}
function Invoke-Alpha([string[]]$Arguments,[string]$LogName){
    # Wait for the GUI bootstrap and its game process. A bare invocation can
    # return before the Windows GUI process and leaves LASTEXITCODE undefined.
    $quoted=@($Arguments | ForEach-Object {
        '"'+[regex]::Replace([regex]::Replace($_,'(\\*)"','$1$1\"'),'(\\+)$','$1$1')+'"'
    })
    $process=Start-Process -FilePath $exe -WorkingDirectory "$archive/Windows" -ArgumentList ($quoted -join ' ') -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput "$audit/$LogName-console.log" -RedirectStandardError "$audit/$LogName-stderr.log"
    if($process.ExitCode -ne 0){throw "Packaged $LogName executable failed with $($process.ExitCode)"}
}
$start=Get-Date
Invoke-Alpha -LogName 'startup' -Arguments @("-UserDir=$userDir/",'-windowed','-ForceRes','-ResX=1920','-ResY=1080','-RecoveryStartupAudit','-RecoveryReconstruction=3','-nosplash','-unattended',"-abslog=$audit/startup.log")
$startupFile=Get-Item -LiteralPath "$saved/startup-report.json"
$startup=Get-Content -Raw -LiteralPath $startupFile.FullName | ConvertFrom-Json
if($startupFile.LastWriteTime -lt $start -or !$startup.ready -or $startup.failed -or $startup.assets_loaded -ne $startup.assets_requested -or $startup.pending_psos_at_reveal -ne 0 -or $startup.pending_textures_at_reveal -ne 0 -or $startup.pending_shaders_at_reveal -ne 0){throw 'Packaged startup revealed before scene readiness'}
Copy-Item -LiteralPath $startupFile.FullName -Destination "$audit/startup.json"
foreach($shot in 'Loading','Home'){if((Get-Item -LiteralPath "$saved/Startup/$shot.png").LastWriteTime -lt $start){throw "No fresh startup $shot capture"}}
Write-Host 'Packaged startup PASS'
$start=Get-Date
Invoke-Alpha -LogName 'controls' -Arguments @("-UserDir=$userDir/",'-windowed','-ForceRes','-ResX=1920','-ResY=1080','-RecoveryControlsAudit','-RecoveryReconstruction=3','-nosplash','-unattended',"-abslog=$audit/controls.log")
$controls=Read-FreshAlphaReport 'ControlsAudit/result.json' $start
Write-Host 'Packaged controls PASS'
$start=Get-Date
Invoke-Alpha -LogName 'flight' -Arguments @("-UserDir=$userDir/",'-windowed','-ForceRes','-ResX=1920','-ResY=1080','-UseFixedTimeStep','-FPS=15','-RecoveryPhysicsAudit','-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1','-RecoveryExperienceAudit','-RecoveryAudioAudit','-RecoveryReview','-RecoveryDetailReview','-RecoveryChaseReview','-RecoveryAutoExit','-RecoveryScenario=Crosswind','-RecoveryReportName=AlphaFlight','-RecoveryReconstruction=3','-nosplash','-unattended',"-abslog=$audit/flight.log")
$flight=Read-FreshAlphaReport 'AlphaFlight.json' $start
$render=Read-FreshAlphaReport 'experience-flight-audit.json' $start
$audio=Get-Item -LiteralPath "$saved/Audio/LaunchMix.wav"
if($audio.LastWriteTime -lt $start){throw 'No fresh packaged audio recording'}
& (Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe') "$root/Tools/Tests/audit_audio_capture.py" $audio.FullName
if($LASTEXITCODE -ne 0){throw 'Packaged launch audio is silent or clipped'}
if(!(Test-RecoveryFrontApproach -Report $flight) -or $flight.solver_support_mask -ne 3 -or !$flight.contact_engine_shutdown -or $flight.unpowered_thrust_violation -or $flight.structural_contacts -ne 0){throw 'Packaged physical capture contract failed'}
if(!$flight.tower_dynamic -or $flight.tower_broken_rail_mask -ne 0 -or $flight.tower_broken_hinge_mask -ne 0 -or !$render.turbulent_volume_budget_pass){throw 'Packaged tower or turbulent volume contract failed'}
if(!(Test-RecoveryGentleContact -Report $flight)){throw 'Packaged return exceeds the gentle-contact or short-burn limits'}
if($render.chase_contact_frames -lt 50 -or $render.chase_contact_max_offset_step_cm -gt .01 -or $render.chase_contact_max_angle_step_deg -gt .001){throw 'Packaged Chase camera is unstable after capture'}
foreach($log in @("$audit/startup.log","$audit/controls.log","$audit/flight.log")){
    if(Select-String -Quiet -LiteralPath $log -Pattern 'Fatal error:|Handled ensure|Ensure condition failed|LogDLSSBlueprint: Error:|Failed to compile Material|Couldn.t find file for package|Failed to find object.*(/Game/|/Starbase/)'){throw "Packaged content failure: $log"}
}
if($render.reconstruction -notmatch 'NVIDIA'){throw 'This DLSS-capable validation machine did not activate DLSS in the packaged game'}
@{success=$true;version=$Version;source_commit=$manifest.source_commit;executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant();startup_seconds=$startup.engine_elapsed_seconds;startup_assets=$startup.assets_loaded;controls_checks=$controls.checks.Count;rendered_frames=$render.frames;capture=$flight.success;front_ingress=$flight.front_ingress_verified;support_mask=$flight.solver_support_mask;support_drift_m=$flight.restraint_drift_m;chase_offset_step_cm=$render.chase_contact_max_offset_step_cm;reconstruction=$render.reconstruction;evidence_directory=$audit;visual_review_required=$true} |
    ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 -LiteralPath "$audit/result.json"
Write-Host "Packaged alpha PASS. Inspect screenshots in $saved. Result: $audit/result.json"

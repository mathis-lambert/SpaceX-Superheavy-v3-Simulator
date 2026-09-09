param(
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$')][string]$Version='0.1.0-alpha.2'
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
Invoke-Alpha -LogName 'controls' -Arguments @("-UserDir=$userDir/",'-windowed','-ForceRes','-ResX=1920','-ResY=1080','-RecoveryControlsAudit','-RecoveryReconstruction=3','-nosplash','-unattended',"-abslog=$audit/controls.log")
$controls=Read-FreshAlphaReport 'ControlsAudit/result.json' $start
Write-Host 'Packaged controls PASS'
$start=Get-Date
Invoke-Alpha -LogName 'flight' -Arguments @("-UserDir=$userDir/",'-windowed','-ForceRes','-ResX=1920','-ResY=1080','-UseFixedTimeStep','-FPS=15','-RecoveryPhysicsAudit','-RecoveryExperienceAudit','-RecoveryReview','-RecoveryChaseReview','-RecoveryAutoExit','-RecoveryScenario=Crosswind','-RecoveryReportName=AlphaFlight','-RecoveryReconstruction=3','-nosplash','-unattended',"-abslog=$audit/flight.log")
$flight=Read-FreshAlphaReport 'AlphaFlight.json' $start
$render=Read-FreshAlphaReport 'experience-flight-audit.json' $start
if(!(Test-RecoveryFrontApproach -Report $flight) -or $flight.solver_support_mask -ne 3 -or !$flight.contact_engine_shutdown -or $flight.unpowered_thrust_violation){throw 'Packaged physical capture contract failed'}
if($render.chase_contact_frames -lt 50 -or $render.chase_contact_max_offset_step_cm -gt .01 -or $render.chase_contact_max_angle_step_deg -gt .001){throw 'Packaged Chase camera is unstable after capture'}
foreach($log in @("$audit/controls.log","$audit/flight.log")){
    if(Select-String -Quiet -LiteralPath $log -Pattern 'Fatal error:|Handled ensure|Ensure condition failed|LogDLSSBlueprint: Error:|Failed to compile Material|Couldn.t find file for package|Failed to find object.*(/Game/|/Starbase/)'){throw "Packaged content failure: $log"}
}
if($render.reconstruction -notmatch 'NVIDIA'){throw 'This DLSS-capable validation machine did not activate DLSS in the packaged game'}
@{success=$true;version=$Version;source_commit=$manifest.source_commit;executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant();controls_checks=$controls.checks.Count;rendered_frames=$render.frames;capture=$flight.success;front_ingress=$flight.front_ingress_verified;support_mask=$flight.solver_support_mask;support_drift_m=$flight.restraint_drift_m;chase_offset_step_cm=$render.chase_contact_max_offset_step_cm;reconstruction=$render.reconstruction;evidence_directory=$audit;visual_review_required=$true} |
    ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 -LiteralPath "$audit/result.json"
Write-Host "Packaged alpha PASS. Inspect screenshots in $saved. Result: $audit/result.json"

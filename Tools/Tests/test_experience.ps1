param(
    [string]$EngineRoot='D:/Engines/UE_5.8',[switch]$SkipMatrix,[switch]$SkipAssets,[switch]$SkipFlight,[switch]$ChaseReview,[switch]$DetailReview,
    [ValidateSet(0,1,2,3,4)][int]$Reconstruction=0,
    [ValidateSet('RecoveryOverhaulAudit','RecoveryUIAudit','RecoveryEarthAudit','RecoveryWorldAudit','RecoveryControlsAudit')]
    [string[]]$MenuAudits=@('RecoveryControlsAudit','RecoveryOverhaulAudit','RecoveryUIAudit','RecoveryEarthAudit')
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project=Join-Path $root 'SuperHeavySim.uproject'
$saved=Join-Path $root 'Saved/Recovery'
. (Join-Path $root 'Tools/Shared/validation_evidence.ps1')
$settings=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$previous=[IO.File]::ReadAllText($settings)
function Confirm-Report([string]$Name,[datetime]$Started){
    $file=Join-Path $saved $Name
    if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $Started){throw "Missing fresh report: $Name"}
    $r=Get-Content -Raw $file | ConvertFrom-Json
    if(!$r.success){throw "Failed report: $Name"}
    Write-Host "$Name PASS"
    return $r
}
try {
    if(!$SkipAssets){
        $started=Get-Date
        & $engine $project -run=PythonScript "-script=$root/Tools/Tests/audit_experience_assets.py" -AllowCommandletRendering -unattended -nosplash -SCCProvider=None "-abslog=$saved/experience-asset-audit.log" *> "$saved/experience-asset-audit-console.log"
        $null=Confirm-Report 'experience-asset-audit.json' $started
    }
    if(!$SkipMatrix){ & (Join-Path $PSScriptRoot 'test_physical_recovery.ps1') -EngineRoot $EngineRoot }
    foreach($case in @(@{Flag='RecoveryControlsAudit';Report='ControlsAudit/result.json'},@{Flag='RecoveryOverhaulAudit';Report='Overhaul/result.json'},@{Flag='RecoveryUIAudit';Report='InterfaceAudit/result.json'},@{Flag='RecoveryEarthAudit';Report='EarthAudit/result.json'},@{Flag='RecoveryWorldAudit';Report='WorldAudit/result.json'})){
        if($case.Flag -notin $MenuAudits){continue}
        $started=Get-Date
        & $engine $project /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ForceRes -ResX=1920 -ResY=1080 "-$($case.Flag)" "-RecoveryReconstruction=$Reconstruction" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/Experience-$($case.Flag).log" *> "$saved/Experience-$($case.Flag)-console.log"
        if(Select-String -Quiet -Path "$saved/Experience-$($case.Flag).log" -Pattern 'Failed to compile Material|Fatal error:'){throw 'Rendered audit used a failed material or crashed'}
        $null=Confirm-Report $case.Report $started
    }
    if($SkipFlight){return}
    $started=Get-Date
    $cameraFlag=if($ChaseReview){'-RecoveryChaseReview'}else{'-RecoveryEarthReview'}
    [string[]]$detailFlag=@()
    if($DetailReview){$detailFlag+=@('-RecoveryDetailReview')}
    & $engine $project /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ForceRes -ResX=1920 -ResY=1080 -UseFixedTimeStep -FPS=15 '-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1' -RecoveryExperienceAudit -RecoveryAudioAudit -RecoveryReview $cameraFlag @detailFlag -RecoveryAutoExit -RecoveryScenario=Crosswind -RecoveryReportName=ExperienceRenderedFlight '-RecoveryHour=17.9' "-RecoveryReconstruction=$Reconstruction" -nosplash -DisablePython -SCCProvider=None "-abslog=$saved/experience-rendered-flight.log" *> "$saved/experience-rendered-flight-console.log"
    if(Select-String -Quiet -Path "$saved/experience-rendered-flight.log" -Pattern 'Failed to compile Material|Fatal error:'){throw 'Rendered flight used a failed material or crashed'}
    $flight=Confirm-Report 'ExperienceRenderedFlight.json' $started
    if(!$flight.left_rail_contact -or !$flight.right_rail_contact -or !$flight.contact_engine_shutdown){throw 'Physical capture contract failed'}
    if(!(Test-RecoveryFrontApproach -Report $flight)){throw 'Front approach corridor contract failed'}
    if(!(Test-RecoveryGentleContact -Report $flight)){throw 'Gentle contact contract failed'}
    $null=Confirm-Report 'experience-flight-audit.json' $started
    $audio=Get-Item -LiteralPath "$saved/Audio/LaunchMix.wav"
    if($audio.LastWriteTime -lt $started){throw 'No fresh audio recording'}
    & (Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe') "$root/Tools/Tests/audit_audio_capture.py" $audio.FullName
    if($LASTEXITCODE -ne 0){throw 'Silent or clipped launch audio'}
} finally {[IO.File]::WriteAllText($settings,$previous)}

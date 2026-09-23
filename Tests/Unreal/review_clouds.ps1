param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [ValidateSet(0,1,3)][int[]]$CloudModes=@(0),
    [ValidateSet(1080,1440,2160)][int]$Height=1440,
    [string]$Prefix='CloudReview',
    [ValidateRange(0,24)][double]$Hour=14
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
. (Join-Path $root 'Tests/Shared/validation_evidence.ps1')
$saved=Join-Path $root 'Saved/Recovery'
$null=New-Item -ItemType Directory -Force -Path $saved
Write-RecoveryBuildEvidence -Root $root -Destination "$saved/$Prefix-source.json" -EngineRoot $EngineRoot
$prefs=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$before=if(Test-Path -LiteralPath $prefs){[IO.File]::ReadAllBytes($prefs)}else{$null}
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$width=[int]($Height*16/9)
try {
    foreach($cloud in $CloudModes) {
        $name="${Prefix}${Height}Mode${cloud}"
        $started=Get-Date
        & $engine "$root/SuperHeavySim.uproject" /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ForceRes "-ResX=$width" "-ResY=$Height" -RecoveryNoMenu -RecoveryAutoExit -RecoveryScenario=Nominal -RecoveryCloudReview -RecoveryWeather=2 -RecoveryEarthReview -UseFixedTimeStep -FPS=30 -RecoveryReconstruction=3 -RecoveryRayTracing=0 "-RecoveryHour=$($Hour.ToString([Globalization.CultureInfo]::InvariantCulture))" "-RecoveryReviewName=$name" "-RecoveryReportName=$name" -DisablePython -nosplash -unattended "-ExecCmds=r.SetRes ${width}x${Height}w,t.MaxFPS 0,r.VSync 0,r.VolumetricRenderTarget.Mode $cloud" "-abslog=$saved/$name.log" *> "$saved/$name-console.log"
        if($LASTEXITCODE -ne 0){throw "Visual flight failed: $name"}
        if(Select-String -Quiet -LiteralPath "$saved/$name.log" -Pattern 'Failed to compile Material|Fatal error:'){throw "Invalid render: $name"}
        $manifest=Get-Item -LiteralPath "$saved/Review/$name/frames.json"
        if($manifest.LastWriteTime -lt $started){throw "Stale visual evidence: $name"}
        $frames=(Get-Content -Raw -LiteralPath $manifest.FullName | ConvertFrom-Json).frames
        if($frames.Count -lt 70){throw "Incomplete moving-view capture: $name"}
        foreach($frame in $frames) {
            $picture=Get-Item -LiteralPath (Join-Path $manifest.DirectoryName $frame.image)
            if($picture.LastWriteTime -lt $started){throw "Stale image: $($picture.Name)"}
            if($frame.'r.VolumetricRenderTarget.Mode' -ne $cloud){throw 'Cloud setting was overridden'}
        }
        Write-Host "Captured $($frames.Count) frames: $($manifest.DirectoryName)"
    }
} finally {
    if($null -ne $before){[IO.File]::WriteAllBytes($prefs,$before)}
    elseif(Test-Path -LiteralPath $prefs){Remove-Item -LiteralPath $prefs}
}

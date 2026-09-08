param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [ValidateSet(0,1,2,3,4)][int[]]$Modes=@(0,3),
    [ValidateSet('Home','Launch')][string[]]$Scenes=@('Home','Launch'),
    [string]$Prefix='Strict',
    [ValidateSet(0,1)][int]$HardwareRayTracing=0,
    [ValidateRange(0,24)][double]$Hour=17.9
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$engine=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$prefs=Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$before=[IO.File]::ReadAllText($prefs)
try {
  foreach($mode in $Modes){
    $label=@('Native','TSRQuality','DLAA','DLSSQuality','DLSSBalanced')[$mode]
    foreach($case in @(@{Name='Home';Frames=1200;Args=@()},@{Name='Launch';Frames=2400;Args=@('-RecoveryNoMenu','-UseFixedTimeStep','-FPS=60')})){
        if($case.Name -notin $Scenes){continue}
        $started=Get-Date
        $extra=$case.Args
        $name="$Prefix$($case.Name)${label}1440"
        & $engine "$root/SuperHeavySim.uproject" /Game/Starbase/Maps/L_RecoveryLab -game -windowed -ForceRes -ResX=2560 -ResY=1440 -DisablePython -nosplash -unattended -csvGpuStats -ExitAfterCsvProfiling @extra "-RecoveryHour=$($Hour.ToString([Globalization.CultureInfo]::InvariantCulture))" "-RecoveryRayTracing=$HardwareRayTracing" "-RecoveryReconstruction=$mode" "-ExecCmds=r.SetRes 2560x1440w,t.MaxFPS 0,r.VSync 0,csvprofile frames=$($case.Frames)" "-abslog=$root/Saved/Recovery/$name.log" *> "$root/Saved/Recovery/$name-console.log"
        if($LASTEXITCODE -ne 0){throw "Benchmark failed: $($case.Name)"}
        if(Select-String -Quiet -Path "$root/Saved/Recovery/$name.log" -Pattern 'Failed to compile Material|Fatal error:'){throw 'Invalid rendering in benchmark'}
        if(!(Select-String -Quiet -Path "$root/Saved/Recovery/$name.log" -Pattern "RECOVERY_RECONSTRUCTION mode=$mode ")){throw "Requested reconstruction unavailable: $label"}
        $csv=Get-ChildItem "$root/Saved/Profiling/CSV" -Filter '*.csv' | Where-Object LastWriteTime -ge $started | Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if(!$csv){throw "No fresh CSV: $($case.Name)"}
        $destination="$root/Saved/Recovery/$name.csv"
        Copy-Item -LiteralPath $csv.FullName -Destination $destination
        Write-Host $destination
    }
  }
} finally {[IO.File]::WriteAllText($prefs,$before)}

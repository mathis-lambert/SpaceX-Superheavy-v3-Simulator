param(
    [string]$EngineRoot = 'D:/Engines/UE_5.8',
    [ValidateRange(640,7680)][int]$Width = 1920,
    [ValidateRange(480,4320)][int]$Height = 1080,
    [switch]$Fullscreen,
    [ValidateRange(0,4)][int]$Reconstruction
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path 'SuperHeavySim.uproject'
$displayArgs = @()
if ($Fullscreen) { $displayArgs += '-fullscreen' }
if ($PSBoundParameters.ContainsKey('Reconstruction')) { $displayArgs += "-RecoveryReconstruction=$Reconstruction" }
if ($PSBoundParameters.ContainsKey('Width') -or $PSBoundParameters.ContainsKey('Height')) {
    $displayArgs += "-ResX=$Width", "-ResY=$Height"
}
# Saved display settings remain authoritative unless explicitly overridden.
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') $project '/Game/Starbase/Maps/L_RecoveryLab' -game @displayArgs -WinX=30 -WinY=30 -nosplash -DisablePython -SCCProvider=None '-ExecCmds=r.MaxAnisotropy 16'

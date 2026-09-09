param([string]$EngineRoot='D:/Engines/UE_5.8')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
# Discover newly added source files even when UBT has a cached makefile.
& (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') SuperHeavySimEditor Win64 Development (Join-Path $root 'SuperHeavySim.uproject') -WaitMutex -NoHotReloadFromIDE -gather
if($LASTEXITCODE -ne 0){throw 'Simulator build failed'}

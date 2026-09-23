param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$')][string]$Version
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (!(Test-Path "$EngineRoot/Engine/Build/BatchFiles/Build.bat")) { throw 'Unreal Engine installation not found' }
git -C $root lfs fsck
if ($LASTEXITCODE) { throw 'Git LFS verification failed' }
& "$PSScriptRoot/build_simulator.ps1" -EngineRoot $EngineRoot
& "$root/Tests/Unreal/Automation/run_model_tests.ps1" -EngineRoot $EngineRoot -Prefix CIPhysics
& "$PSScriptRoot/package_alpha.ps1" -EngineRoot $EngineRoot -Version $Version
python "$PSScriptRoot/archive_alpha.py" "$root/Releases/Starbase-$Version"
if ($LASTEXITCODE) { throw 'Release archive validation failed' }
$upload = "$root/Releases/upload"
if (Test-Path $upload) { throw 'Release upload directory already exists; use a clean build workspace' }
$null = New-Item -ItemType Directory -Path $upload
foreach ($suffix in @('.zip', '.zip.sha256', '-artifacts.json')) {
    Copy-Item -LiteralPath "$root/Releases/Starbase-$Version$suffix" -Destination $upload
}

param(
    [string]$EngineRoot='D:/Engines/UE_5.8',
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$')][string]$Version='0.1.0-alpha.4'
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project=Join-Path $root 'SuperHeavySim.uproject'
$archive=Join-Path $root "Releases/Starbase-$Version"
$evidence=Join-Path $root 'Saved/Recovery'
$null=New-Item -ItemType Directory -Force -Path $evidence
if(Test-Path -LiteralPath $archive){throw 'This alpha directory already exists. Use a new alpha version to preserve it.'}
$configured=Select-String -LiteralPath "$root/Config/DefaultGame.ini" -Pattern '^ProjectVersion=(.+)$'
if(!$configured -or $configured.Matches[0].Groups[1].Value -ne $Version){throw 'Package version must match ProjectVersion in DefaultGame.ini'}
$sourceCommit=(& git -c "safe.directory=$($root.Replace('\','/'))" -C $root rev-parse HEAD).Trim()
$sourceChanges=@(& git -c "safe.directory=$($root.Replace('\','/'))" -C $root status --porcelain)
if($LASTEXITCODE -ne 0 -or $sourceChanges.Count -gt 0){throw 'Commit the release sources before packaging'}
$started=Get-Date
$log=Join-Path $evidence "Alpha-$Version-package.log"
# Development game build preserves the interactive force inspector and local
# diagnostics. The Editor target and its MCP/toolset plugins are excluded.
& (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$project" -target=SuperHeavySim -noP4 -platform=Win64 -clientconfig=Development -installed -nocompileeditor -skipbuildeditor -build -cook -stage -pak -iostore -compressed -prereqs -nodebuginfo -archive "-archivedirectory=$archive" -ubtargs=-gather -utf8output -unattended *> $log
if($LASTEXITCODE -ne 0){Get-Content -LiteralPath $log -Tail 50;throw "Alpha packaging failed. See $log"}
$executable=Join-Path $archive 'Windows/SuperHeavySim.exe'
if(!(Test-Path -LiteralPath $executable) -or (Get-Item -LiteralPath $executable).LastWriteTime -lt $started){throw 'Packaging produced no fresh standalone executable'}
Copy-Item -LiteralPath "$root/Docs/Releases/ALPHA_0.1.0.md" -Destination "$archive/README.md"
Copy-Item -LiteralPath "$root/Docs/SITE_PHOTOGRAPHY.md" -Destination "$archive/PHOTOGRAPHY.md"
Copy-Item -LiteralPath "$root/Docs/DYNAMIC_RETURN.md" -Destination "$archive/FLIGHT.md"
$nvidiaLicense=Join-Path $root '../ArtSource/ThirdParty/NVIDIA/LICENSE.txt'
if(Test-Path -LiteralPath $nvidiaLicense){
    $null=New-Item -ItemType Directory -Path "$archive/ThirdParty" -Force
    Copy-Item -LiteralPath $nvidiaLicense -Destination "$archive/ThirdParty/NVIDIA-LICENSE.txt"
}
$files=@(Get-ChildItem -LiteralPath $archive -File -Recurse | ForEach-Object {
    @{path=[IO.Path]::GetRelativePath($archive,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
@{version=$Version;source_commit=$sourceCommit;configuration='Development';platform='Windows x64';engine='Unreal Engine 5.8';created_utc=(Get-Date).ToUniversalTime().ToString('o');executable='Windows/SuperHeavySim.exe';files=$files} |
    ConvertTo-Json -Depth 6 | Set-Content -Encoding utf8 -LiteralPath "$archive/build-manifest.json"
Write-Host "Alpha packaged: $executable"

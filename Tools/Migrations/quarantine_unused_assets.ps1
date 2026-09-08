param([string]$Inventory='')
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if(!$Inventory){$Inventory=Join-Path $projectRoot 'Saved/Recovery/unused-asset-inventory.json'}
$report=Get-Content -LiteralPath $Inventory -Raw | ConvertFrom-Json
if(!$report.success -or $report.external_referencers.Count -gt 0){throw 'Dependency inventory is not clear'}
$contentRoot=[IO.Path]::GetFullPath((Join-Path $projectRoot 'Content'))+[IO.Path]::DirectorySeparatorChar
$archiveRoot=[IO.Path]::GetFullPath((Join-Path $projectRoot 'Saved/Recovery/UnusedContent'))+[IO.Path]::DirectorySeparatorChar
$checked=@()
foreach($entry in $report.files){
    $source=[IO.Path]::GetFullPath((Join-Path $projectRoot $entry.path))
    $destination=[IO.Path]::GetFullPath((Join-Path $archiveRoot $entry.path))
    if(!$source.StartsWith($contentRoot,[StringComparison]::OrdinalIgnoreCase) -or !$destination.StartsWith($archiveRoot,[StringComparison]::OrdinalIgnoreCase)){throw "Path outside intended roots: $($entry.path)"}
    if([IO.Path]::GetExtension($source) -notin @('.uasset','.umap','.ubulk','.uexp')){throw 'Unexpected file type'}
    if(!(Test-Path -LiteralPath $source) -or (Get-Item -LiteralPath $source).Length -ne $entry.bytes){throw "Inventory changed: $source"}
    if(Test-Path -LiteralPath $destination){throw "Recovery destination already exists: $destination"}
    $checked += [pscustomobject]@{source=$source;destination=$destination;sha256=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash}
}
# Verify every recovery copy before removing any active package. All operations
# stay in PowerShell and use the fully checked literal paths above.
foreach($file in $checked){
    New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($file.destination)) -Force | Out-Null
    Copy-Item -LiteralPath $file.source -Destination $file.destination
    if((Get-FileHash -LiteralPath $file.destination -Algorithm SHA256).Hash -ne $file.sha256){throw "Copy mismatch: $($file.source)"}
}
$checked | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $archiveRoot 'manifest.json')
Copy-Item -LiteralPath $Inventory -Destination (Join-Path $archiveRoot 'dependency-inventory.json')
foreach($file in $checked){Remove-Item -LiteralPath $file.source}
Write-Output "Quarantined $($checked.Count) unused package files; verified recovery copies retained."

function Test-RecoveryFrontApproach {
    param([object]$Report)
    return $Report.front_approach_samples -gt 0 -and $Report.front_ingress_verified -and $null -ne $Report.front_min_mast_clearance_m -and $null -ne $Report.front_min_corridor_margin_m -and $Report.front_min_mast_clearance_m -ge 0 -and $Report.front_min_corridor_margin_m -ge 0
}

function Test-RecoveryGentleContact {
    param([object]$Report)
    # Measured incoming solver state, before support removes momentum.
    return $Report.first_contact_time_s -gt $Report.landing_ignition_time_s -and `
        $Report.landing_burn_seconds -lt 45 -and $Report.low_slow_approach_seconds -lt 8 -and `
        $Report.first_contact_speed_mps -lt .85 -and `
        $null -ne $Report.first_contact_vertical_speed_mps -and [math]::Abs($Report.first_contact_vertical_speed_mps) -lt .35 -and `
        $Report.first_contact_tilt_deg -lt 1.2 -and `
        $null -ne $Report.first_contact_angular_speed_deg_s -and $Report.first_contact_angular_speed_deg_s -lt 3.2
}

function Write-RecoveryBuildEvidence {
    param([string]$Root,[string]$Destination,[string]$EngineRoot)
    $files=@(Get-Item -LiteralPath (Join-Path $Root 'SuperHeavySim.uproject'),(Join-Path $Root 'Binaries/Win64/UnrealEditor-SuperHeavySim.dll'))
    foreach($folder in @('Source','Content','Config','Tools','Tests')) {
        $files+=Get-ChildItem -LiteralPath (Join-Path $Root $folder) -File -Recurse |
            Where-Object { $_.Extension -notin @('.pyc','.pyo') }
    }
    $entries=@($files | Sort-Object FullName | ForEach-Object {
        @{path=[IO.Path]::GetRelativePath($Root,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    })
    $evidence=@{
        schema_version=1
        utc_recorded=(Get-Date).ToUniversalTime().ToString('o')
        git_head=(& git -c "safe.directory=$($Root.Replace('\','/'))" -C $Root rev-parse HEAD)
        git_status=@(& git -c "safe.directory=$($Root.Replace('\','/'))" -C $Root status --porcelain)
        engine=(Get-Content -Raw -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') | ConvertFrom-Json)
        user_settings=if(Test-Path -LiteralPath "$Root/Saved/Config/WindowsEditor/GameUserSettings.ini"){Get-Content -Raw -LiteralPath "$Root/Saved/Config/WindowsEditor/GameUserSettings.ini"}else{$null}
        files=$entries
        scope='Project source, authored content, config, tools, tests, project descriptor and loaded game-module DLL. Engine and vendor binaries are not hashed.'
    }
    $evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding utf8 -LiteralPath $Destination
}

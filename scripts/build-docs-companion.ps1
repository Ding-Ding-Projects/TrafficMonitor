param([string]$Version = '1.0.0')
. "$PSScriptRoot/build-tools.ps1"
try {
    if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Documentation version must be a three-part numeric version' }
    & "$RepoRoot/build.bat" --site
    if ($LASTEXITCODE -ne 0) { throw "Documentation production failed with exit $LASTEXITCODE" }
    $runtime = Join-Path $ToolsRoot 'node-v24.10.0-win-x64'
    $node = Get-Command node.exe -ErrorAction SilentlyContinue
    $nodePath = if ($node -and (& $node.Source --version) -eq 'v24.10.0') { $node.Source } else { Join-Path $runtime 'node.exe' }
    if (!(Test-Path -LiteralPath $nodePath)) { throw 'Pinned documentation runtime was not acquired by the site entrypoint' }
    $npm = Join-Path (Split-Path $nodePath) 'node_modules/npm/bin/npm-cli.js'
    $companion = Join-Path $RepoRoot 'docs-companion'
    $env:PATH = (Split-Path $nodePath) + [IO.Path]::PathSeparator + $env:PATH
    Push-Location $companion
    try {
        & $nodePath $npm ci --ignore-scripts --no-audit --no-fund
        if ($LASTEXITCODE -ne 0) { throw "Locked companion dependency acquisition failed with exit $LASTEXITCODE" }
        & $nodePath stage.mjs
        if ($LASTEXITCODE -ne 0) { throw 'Offline staging failed' }
        & $nodePath node_modules/electron-builder/cli.js --win portable --x64 --publish never "-c.extraMetadata.version=$Version"
        if ($LASTEXITCODE -ne 0) { throw "Portable documentation production failed with exit $LASTEXITCODE" }
        $file = Join-Path $RepoRoot "artifacts/docs-companion/TrafficMonitor-Documentation-$Version-x64.exe"
        if (!(Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).Length -lt 1MB) { throw 'Portable output is missing or unexpectedly small' }
        $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        $source = (& git -C $RepoRoot rev-parse HEAD).Trim()
        $manifest = @{schemaVersion=1;sourceCommit=$source;version=$Version;file=(Split-Path $file -Leaf);bytes=(Get-Item $file).Length;sha256=$hash;runtimeVersion='45.0.0';builderVersion='26.15.3';format='single-file portable documentation companion';runtimeVerification='pending';offlineInventorySHA256=(Get-FileHash site/offline-inventory.json -Algorithm SHA256).Hash.ToLowerInvariant()}
        $manifest | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $RepoRoot 'artifacts/docs-companion/documentation-manifest.json')
        Copy-Item site/offline-inventory.json (Join-Path $RepoRoot 'artifacts/docs-companion/offline-inventory.json')
        "$hash  $(Split-Path $file -Leaf)" | Set-Content -Encoding ascii (Join-Path $RepoRoot 'artifacts/docs-companion/documentation-SHA256SUMS.txt')
        Write-Host "Produced unsigned portable documentation companion: $file SHA256 $hash. Runtime verification pending."
    } finally { Pop-Location }
} catch { Write-Error $_; exit 1 }

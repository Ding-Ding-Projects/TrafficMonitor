. "$PSScriptRoot/build-tools.ps1"
try {
    $entrypoint = Join-Path $RepoRoot 'docs/site/build.mjs'
    if (!(Test-Path -LiteralPath $entrypoint)) { throw 'Documentation build entrypoint is not yet available' }
    $node = Get-Command node.exe -ErrorAction SilentlyContinue
    $nodePath = if ($node) { $node.Source } else { $null }
    if (!$nodePath -or (& $nodePath --version) -ne 'v24.10.0' -or !(Test-Path -LiteralPath (Join-Path (Split-Path $nodePath) 'node_modules/npm/bin/npm-cli.js'))) {
        $archive = Join-Path $ToolsRoot 'node-v24.10.0-win-x64.zip'
        Get-TrustedFile 'https://nodejs.org/dist/v24.10.0/node-v24.10.0-win-x64.zip' $archive 'adc1a2d5ca79c92e94f3a58c3ec0efa76bdb488769ba4d4b50990e4c84896060'
        $runtime = Join-Path $ToolsRoot 'node-v24.10.0-win-x64'
        if (!(Test-Path -LiteralPath "$runtime/node.exe")) { Expand-Archive -LiteralPath $archive -DestinationPath $ToolsRoot -Force }
        $nodePath = Join-Path $runtime 'node.exe'
    }
    if ((& $nodePath --version) -ne 'v24.10.0') { throw 'Pinned documentation runtime verification failed' }
    Push-Location $RepoRoot
    try {
        $npm = Join-Path (Split-Path $nodePath) 'node_modules/npm/bin/npm-cli.js'
        if (!(Test-Path -LiteralPath "$RepoRoot/docs/site/package-lock.json")) { throw 'Documentation dependency lock is not yet available' }
        & $nodePath $npm ci --prefix "$RepoRoot/docs/site" --include=dev --include=optional --ignore-scripts --no-audit --no-fund
        if ($LASTEXITCODE -ne 0) { throw "Documentation dependency installation failed with exit $LASTEXITCODE" }
        & $nodePath "$RepoRoot/scripts/refresh-site-content.mjs" --output artifacts/documentation-content-manifest.json
        if ($LASTEXITCODE -ne 0) { throw 'Current documentation inventory generation failed' }
        & $nodePath $entrypoint --manifest artifacts/documentation-content-manifest.json
        if ($LASTEXITCODE -ne 0) { throw "Documentation build failed with exit $LASTEXITCODE" }
        if (!(Test-Path -LiteralPath "$RepoRoot/docs/site/dist/index.html")) { throw 'Documentation home page was not produced' }
    } finally { Pop-Location }
} catch { Write-Error $_; exit 1 }

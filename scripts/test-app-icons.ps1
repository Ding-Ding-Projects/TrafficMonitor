param([string]$Companion = '', [string]$Native = '')
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $runtime = Join-Path $ToolsRoot 'node-v24.10.0-win-x64'
    $node = Get-Command node.exe -ErrorAction SilentlyContinue
    $nodePath = if ($node -and (& $node.Source --version) -eq 'v24.10.0') { $node.Source } else { Join-Path $runtime 'node.exe' }
    if (!(Test-Path -LiteralPath $nodePath)) { throw 'Pinned documentation runtime is unavailable; run build.bat --docs-companion first' }
    if (!$Native) { $Native = Join-Path $RepoRoot 'Bin/x64/Release (lite)/TrafficMonitor.exe' }
    if (!$Companion) {
        $latest = Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'artifacts/docs-companion') -Filter 'TrafficMonitor-Documentation-*-x64.exe' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if (!$latest) { throw 'Documentation companion is not built; run build-installer.bat --docs-companion first' }
        $Companion = $latest.FullName
    }
    foreach ($file in @($Native, $Companion)) { if (!(Test-Path -LiteralPath $file)) { throw "Built executable is missing: $file" } }
    $arguments = @((Join-Path $PSScriptRoot 'test-app-icons.mjs'), '--ico', (Join-Path $RepoRoot 'TrafficMonitor/res/TrafficMonitor.ico'))
    # The pristine framework executable is read from the cached Electron archive that the build downloaded.
    $scratch = Join-Path ([IO.Path]::GetTempPath()) ('app-icons-' + [guid]::NewGuid().ToString('N'))
    $archive = Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'electron/Cache') -Recurse -Filter 'electron-v*-win32-x64.zip' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($archive) {
        New-Item -ItemType Directory -Path $scratch | Out-Null
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [IO.Compression.ZipFile]::OpenRead($archive.FullName)
        try { [IO.Compression.ZipFileExtensions]::ExtractToFile($zip.GetEntry('electron.exe'), (Join-Path $scratch 'electron.exe')) } finally { $zip.Dispose() }
        $arguments += @('--reject-default', (Join-Path $scratch 'electron.exe'))
    } else { Write-Host 'Cached framework archive not found; default-icon comparison skipped.' }
    $arguments += @('--exe', "native=$Native", '--exe', "documentation-companion=$Companion")
    & $nodePath @arguments
    $code = $LASTEXITCODE
    if (Test-Path -LiteralPath $scratch) { Remove-Item -LiteralPath $scratch -Recurse -Force }
    exit $code
} catch { Write-Error $_; exit 1 }
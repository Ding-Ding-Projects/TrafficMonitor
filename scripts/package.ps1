param([string]$Version = '1.0.0')
. "$PSScriptRoot/build-tools.ps1"
try {
    if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Version must be three numeric SemVer components' }
    $installation = Get-BuildInstallation
    $squirrelArchive = Join-Path $ToolsRoot 'squirrel.windows.2.0.1.zip'
    Get-TrustedFile 'https://api.nuget.org/v3-flatcontainer/squirrel.windows/2.0.1/squirrel.windows.2.0.1.nupkg' $squirrelArchive '923E18ABB4FD50B5A4878A39DBCD042ED3F7EB68FC0F82C0955CD5380C921AC7'
    $squirrel = Join-Path $ToolsRoot 'squirrel.windows.2.0.1'
    if (!(Test-Path "$squirrel/tools/Squirrel.exe")) { Expand-Archive $squirrelArchive $squirrel -Force }
    $stage = Join-Path $ToolsRoot ("package-" + [Guid]::NewGuid().ToString('N'))
    $payload = "$stage/lib/net45"
    New-Item -ItemType Directory -Force $payload,"$RepoRoot/artifacts/releases" | Out-Null
    Copy-Item "$RepoRoot/Bin/x64/Release (lite)/TrafficMonitor.exe" $payload
    foreach ($directory in @('language','skins')) {
        if (Test-Path "$RepoRoot/TrafficMonitor/$directory") { Copy-Item "$RepoRoot/TrafficMonitor/$directory" $payload -Recurse }
    }
    Copy-Item "$RepoRoot/LICENSE" $payload
    # App-local redistribution installs the matching runtime with the application,
    # preserving system-wide runtimes and requiring no separate elevated installer.
    $redist = Get-ChildItem "$installation/VC/Redist/MSVC" -Directory | Where-Object Name -match '^14\.' | Sort-Object Name -Descending | Select-Object -First 1
    if (!$redist) { throw 'MSVC redistributable directory is missing' }
    foreach ($name in @('Microsoft.VC143.CRT','Microsoft.VC143.MFC')) {
        $runtime = Join-Path $redist.FullName "x64/$name"
        if (!(Test-Path $runtime)) { throw "Required redistributable missing: $name" }
        Copy-Item "$runtime/*.dll" $payload
    }
    foreach ($dll in @('mfc140u.dll','vcruntime140.dll','vcruntime140_1.dll','msvcp140.dll')) {
        if (!(Test-Path "$payload/$dll")) { throw "Runtime readiness check failed: $dll" }
    }
    "[installation]`r`nmanaged=true`r`n" | Set-Content "$payload/installed-mode.ini" -Encoding ASCII
    $sha = (& git -C $RepoRoot rev-parse HEAD).Trim()
    @{sourceCommit=$sha;version=$Version;architecture='x64';configuration='Release (lite)';runtime='MSVC v143 CRT and MFC app-local';squirrel='2.0.1'} | ConvertTo-Json | Set-Content "$payload/build-manifest.json" -Encoding UTF8
    @"
<?xml version="1.0"?>
<package><metadata><id>TrafficMonitor</id><version>$Version</version><authors>TrafficMonitor contributors</authors><description>Native TrafficMonitor x64 with application-local Microsoft CRT and MFC runtime.</description><title>TrafficMonitor</title></metadata></package>
"@ | Set-Content "$stage/TrafficMonitor.nuspec" -Encoding UTF8
    $nuget = Join-Path $ToolsRoot 'nuget-6.12.1.exe'
    Get-TrustedFile 'https://dist.nuget.org/win-x86-commandline/v6.12.1/nuget.exe' $nuget '0790BB7A0C898E44B70F2B65E3070B4DB8AF23897E38B8653D72D268B6E8BB11'
    $inputPackage = "$RepoRoot/artifacts/TrafficMonitor.$Version.nupkg"
    & $nuget pack "$stage/TrafficMonitor.nuspec" -BasePath $stage -OutputDirectory "$RepoRoot/artifacts" -NoPackageAnalysis -NonInteractive
    if ($LASTEXITCODE -ne 0) { throw "NuGet packing failed: $LASTEXITCODE" }
    & "$squirrel/tools/Squirrel.exe" --releasify $inputPackage --releaseDir "$RepoRoot/artifacts/releases" --no-msi --framework-version net48
    if ($LASTEXITCODE -ne 0) { throw "Squirrel packaging failed: $LASTEXITCODE" }
    foreach ($file in @('Setup.exe','RELEASES',"TrafficMonitor-$Version-full.nupkg")) {
        if (!(Test-Path "$RepoRoot/artifacts/releases/$file")) { throw "Squirrel output missing: $file" }
    }
    Get-ChildItem "$RepoRoot/artifacts/releases" -File | ForEach-Object { "$((Get-FileHash $_.FullName -Algorithm SHA256).Hash)  $($_.Name)" } | Set-Content "$RepoRoot/artifacts/releases/SHA256SUMS.txt"
} catch { Write-Error $_; exit 1 }

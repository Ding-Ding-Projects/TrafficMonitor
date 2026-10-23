param([switch]$Silent)
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation = Get-BuildInstallation
    $msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
    New-Item -ItemType Directory -Force "$RepoRoot/artifacts/logs" | Out-Null
    Push-Location $RepoRoot
    try {
        & $msbuild TrafficMonitor_Lite.sln /m /t:Build '/p:Configuration=Release (lite)' /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0 "/flp:logfile=$RepoRoot/artifacts/logs/build.log;verbosity=normal"
        if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit $LASTEXITCODE" }
        $exe = "$RepoRoot/Bin/x64/Release (lite)/TrafficMonitor.exe"
        if (!(Test-Path $exe)) { throw 'Expected native x64 executable missing' }
        Write-Host "Build completed: $exe"
    } finally { Pop-Location }
} catch { Write-Error $_; exit 1 }

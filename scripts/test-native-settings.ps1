$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-settings-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $exe=Join-Path $work 'DraftValidationTests.exe'
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativeSettings/DraftValidationTests.cpp'
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /W4 /WX "'+$source+'" /Fe:"'+$exe+'"'
    Push-Location $work
    try { & cmd.exe /d /c $command; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){exit $compileExit}
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

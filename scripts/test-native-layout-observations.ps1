$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-layout-observations-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $exe=Join-Path $work 'LayoutObservationTests.exe'
    $sources=@('LayoutObservationTests.cpp','LayoutObservations.cpp','ButtonVisual.cpp','ButtonDrawTextBaseline.cpp','Theme.cpp','ScrollRail.cpp','ScrollViewport.cpp') | ForEach-Object {'"'+(Join-Path $RepoRoot ('TrafficMonitor/NativeMaterial/'+$_))+'"'}
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE '+($sources -join ' ')+' /Fe:"'+$exe+'" /link UIAutomationCore.lib'
    Push-Location $work
    try { & cmd.exe /d /c $command; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){exit $compileExit}
    $off=& $exe; $offExit=$LASTEXITCODE
    $off | ForEach-Object { Write-Host $_ }
    if($offExit -ne 0){exit $offExit}
    # The marker exists only beside this owned test executable, never a package.
    New-Item -ItemType File -Path ($exe+'.layout-observations') | Out-Null
    $on=& $exe --expect-enabled; $onExit=$LASTEXITCODE
    $on | ForEach-Object { Write-Host $_ }
    if($onExit -ne 0){exit $onExit}
    $offHash=@($off | Where-Object {$_ -like 'PIXEL_CHECKSUM *'})
    $onHash=@($on | Where-Object {$_ -like 'PIXEL_CHECKSUM *'})
    if($offHash.Count -ne 1 -or $onHash.Count -ne 1 -or $offHash[0] -cne $onHash[0]){throw 'Observation opt-in changed rendered pixels'}
    Write-Host 'PASS opt-in on/off rendered pixel parity'
    exit 0
} catch { Write-Error $_; exit 1 }

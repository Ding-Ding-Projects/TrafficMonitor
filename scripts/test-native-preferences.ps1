$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-preferences-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $exe=Join-Path $work 'PreferencesTests.exe'
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativePreferences/PreferencesTests.cpp'
    $storage=Join-Path $RepoRoot 'TrafficMonitor/NativePreferences/Storage.cpp'
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /W4 /WX "'+$source+'" "'+$storage+'" /Fe:"'+$exe+'"'
    Push-Location $work
    try { & cmd.exe /d /c $command; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){exit $compileExit}
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Exercise the same INI reader, writer and preference initialization used by the application.
    $setupExe=Join-Path $work 'SetupSerializationTests.exe'
    $sources=@('NativePreferences/SetupSerializationTests.cpp','IniHelper.cpp','Common.cpp','NativePreferences/Preferences.cpp','NativePreferences/Subscription.cpp','NativePreferences/Storage.cpp') | ForEach-Object {'"'+(Join-Path $RepoRoot ('TrafficMonitor/'+$_))+'"'}
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /MD /Gy /O2 /Zc:inline /D_AFXDLL /D_UNICODE /DUNICODE /DWITHOUT_TEMPERATURE /I"'+(Join-Path $RepoRoot 'include')+'" '+($sources -join ' ')+' /Fe:"'+$setupExe+'" /link /OPT:REF'
    Push-Location $work
    try { & cmd.exe /d /c $command; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){exit $compileExit}
    & $setupExe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    $header=Join-Path $RepoRoot 'TrafficMonitor/NativePreferences/Draft.h'
    $original=[IO.File]::ReadAllText($header)
    $needle='if(!draft.dirty)'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected one resume preservation condition'}
    [IO.File]::WriteAllText((Join-Path $work 'Draft.h'),$original.Replace($needle,'if(true)'))
    Copy-Item -LiteralPath $source -Destination (Join-Path $work 'PreferencesTests.cpp')
    Copy-Item -LiteralPath (Join-Path $RepoRoot 'TrafficMonitor/NativePreferences/Preferences.h') -Destination $work
    Copy-Item -LiteralPath (Join-Path $RepoRoot 'TrafficMonitor/NativePreferences/Storage.h') -Destination $work
    $mutant=Join-Path $work 'MutatedPreferencesTests.exe'
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /W4 /WX "'+(Join-Path $work 'PreferencesTests.cpp')+'" "'+$storage+'" /Fe:"'+$mutant+'"'
    Push-Location $work
    try { & cmd.exe /d /c $command; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){exit $compileExit}
    $output=& $mutant
    $result=$LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if($result -ne 1 -or ($output -join "`n") -notmatch 'FAIL saved draft A survives accepted B'){throw 'Resume mutation was not rejected by the expected regression'}
    if([IO.File]::ReadAllText($header) -cne $original){throw 'Production header changed during mutation verification'}
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

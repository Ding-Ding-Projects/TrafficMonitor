param([switch]$Baseline)
. "$PSScriptRoot/build-tools.ps1"
$installation=Get-BuildInstallation
$version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
$compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
$work=Join-Path $RepoRoot ('artifacts/protocol-'+[guid]::NewGuid().ToString('N'))
$app=Join-Path $work 'app-1.0.31'
New-Item -ItemType Directory -Path $app | Out-Null
$source=Join-Path $RepoRoot 'TrafficMonitor/NativeUpdate'
$defines=@()
if($Baseline){
    $source=Join-Path $work 'baseline/NativeUpdate';New-Item -ItemType Directory -Path $source -Force|Out-Null
    foreach($name in 'SilentUpdater.cpp','SilentUpdater.h','HelperProtocol.h','Transport.h','Json.h'){
        $bytes=& git show "2098090699c6029c47fb1e432742b7cc51834350`:TrafficMonitor/NativeUpdate/$name"
        [IO.File]::WriteAllText((Join-Path $source $name),($bytes -join "`n"),[Text.UTF8Encoding]::new($false))
    }
    Copy-Item -LiteralPath "$RepoRoot/TrafficMonitor/InstallerLifecycle.h" -Destination (Join-Path (Split-Path $source) 'InstallerLifecycle.h')
    $defines=@('/DNATIVE_UPDATE_BASELINE')
}
$vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
$exe=Join-Path $app 'TrafficMonitor.exe'
$test=Join-Path $RepoRoot 'tests/native-update-protocol.cpp'
$transport=Join-Path $RepoRoot 'TrafficMonitor/NativeUpdate/Transport.cpp'
$command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE '+($defines -join ' ')+' /I"'+$source+'" "'+$test+'" "'+$transport+'" /Fe:"'+$exe+'" /link winhttp.lib bcrypt.lib shell32.lib advapi32.lib user32.lib'
Push-Location $work
& cmd.exe /d /c $command
$compileExit=$LASTEXITCODE
Pop-Location
if($compileExit -ne 0){throw 'Protocol check compilation failed'}
[IO.File]::WriteAllText((Join-Path $app 'installed-mode.ini'),"[installation]`nmanaged=true`n")
[IO.File]::WriteAllText((Join-Path $app 'build-manifest.json'),'{"version":"1.0.31","architecture":"x64","squirrel":"2.0.1"}')
[IO.File]::WriteAllText((Join-Path $work 'Update.exe'),'Not executable: no actual updater may be launched in this local fixture.')
& $exe
$normal=$LASTEXITCODE
$failureApp=Join-Path $work 'create-failure/app-1.0.31';New-Item -ItemType Directory -Path $failureApp -Force|Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $failureApp 'Current.exe')
Copy-Item -LiteralPath (Join-Path $app 'installed-mode.ini'),(Join-Path $app 'build-manifest.json') -Destination $failureApp
Copy-Item -LiteralPath (Join-Path $work 'Update.exe') -Destination (Split-Path $failureApp)
& (Join-Path $failureApp 'Current.exe') create-failure
$create=$LASTEXITCODE
Write-Host "Protocol results: preparation=$normal creation=$create; fixture=$work"
if($normal -ne 0 -or $create -ne 0){exit 1}
exit 0

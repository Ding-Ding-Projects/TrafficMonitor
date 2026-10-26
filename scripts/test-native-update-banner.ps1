$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
$installation=Get-BuildInstallation
$version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
$compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
$vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
$work=Join-Path $RepoRoot ('artifacts/update-banner-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
$test=Join-Path $RepoRoot 'tests/native-update-banner.cpp'
function Build-Check($include,$name) {
    $exe=Join-Path $work ($name+'.exe')
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE /I"'+$include+'" "'+$test+'" /Fe:"'+$exe+'"'
    Push-Location $work
    try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){throw "Banner check compilation failed ($name)"}
    return $exe
}
$source=Join-Path $RepoRoot 'TrafficMonitor/NativeUpdate'
$exe=Build-Check $source 'BannerChecks'
$green=& $exe
$greenExit=$LASTEXITCODE
$green | ForEach-Object { Write-Host $_ }
if($greenExit -ne 0){exit 1}

# Disposable mutation: let automatic restart ignore its preference so a restart can start without the person's choice.
$mutantDir=Join-Path $work 'mutant'
New-Item -ItemType Directory -Path $mutantDir | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'SilentUpdater.h'),(Join-Path $source 'BannerLogic.h') -Destination $mutantDir
$logic=Join-Path $mutantDir 'BannerLogic.h'
$text=[IO.File]::ReadAllText($logic)
$needle='return automaticRestartPreference && !ui.deferred;'
if(($text.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected exactly one automatic restart rule'}
[IO.File]::WriteAllText($logic,$text.Replace($needle,'return !ui.deferred;'),[Text.UTF8Encoding]::new($true))
$mutant=Build-Check $mutantDir 'MutatedBannerChecks'
$red=& $mutant
$redExit=$LASTEXITCODE
$red | ForEach-Object { Write-Host $_ }
if($redExit -ne 1 -or ($red -join "`n") -notmatch 'FAIL no restart without a choice'){throw 'Restart-without-choice mutation was not rejected'}
Write-Host 'Expected rejection: disposable restart-without-choice mutation returned 1.'
Write-Host "Banner results: green=$greenExit mutation=$redExit; fixture=$work"
exit 0

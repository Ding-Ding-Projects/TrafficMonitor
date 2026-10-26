$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
$installation=Get-BuildInstallation
$version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
$compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
$vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
$work=Join-Path $RepoRoot ('artifacts/diagnostic-instance-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
$test=Join-Path $RepoRoot 'tests/native-diagnostic-instance.cpp'
function Build-Check($include,$name) {
    $exe=Join-Path $work ($name+'.exe')
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE /I"'+$include+'" "'+$test+'" /Fe:"'+$exe+'"'
    Push-Location $work
    try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){throw "Diagnostic instance check compilation failed ($name)"}
    return $exe
}
$source=Join-Path $RepoRoot 'TrafficMonitor'
$exe=Build-Check $source 'DiagnosticInstanceChecks'
$green=& $exe
$greenExit=$LASTEXITCODE
$green | ForEach-Object { Write-Host $_ }
if($greenExit -ne 0){exit 1}

# Disposable mutation: drop the portable-mode requirement so a marker isolates even the installed copy.
$mutantDir=Join-Path $work 'mutant'
New-Item -ItemType Directory -Path $mutantDir | Out-Null
$header=Join-Path $mutantDir 'DiagnosticInstance.h'
$text=[IO.File]::ReadAllText((Join-Path $source 'DiagnosticInstance.h'))
$needle='return markerIsRegularFile && !installedMode;'
if(($text.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected exactly one portable-mode rule'}
[IO.File]::WriteAllText($header,$text.Replace($needle,'return markerIsRegularFile;'),[Text.UTF8Encoding]::new($true))
$mutant=Build-Check $mutantDir 'MutatedDiagnosticInstanceChecks'
$red=& $mutant
$redExit=$LASTEXITCODE
$red | ForEach-Object { Write-Host $_ }
if($redExit -ne 1 -or ($red -join "`n") -notmatch 'FAIL marker in installed mode keeps the normal name'){throw 'Installed-mode isolation mutation was not rejected'}
Write-Host 'Expected rejection: disposable installed-mode isolation mutation returned 1.'
Write-Host "Diagnostic instance results: green=$greenExit mutation=$redExit; fixture=$work"
exit 0
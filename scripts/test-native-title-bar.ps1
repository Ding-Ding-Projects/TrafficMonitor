$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-title-bar-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    function Build-Test($titleBar,$name) {
        $exe=Join-Path $work ($name+'.exe')
        $sources=@('TitleBarTests.cpp','Theme.cpp') | ForEach-Object {'"'+(Join-Path $RepoRoot ('TrafficMonitor/NativeMaterial/'+$_))+'"'}
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I"'+(Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial')+'" '+($sources -join ' ')+' "'+$titleBar+'" /Fe:"'+$exe+'" /link UIAutomationCore.lib'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
        if($compileExit -ne 0){throw "Compilation returned $compileExit"}
        return $exe
    }
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial/TitleBar.cpp'
    $original=[IO.File]::ReadAllText($source)
    $exe=Build-Test $source 'TitleBarTests'
    & $exe | ForEach-Object { Write-Host $_ }
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Break the maximize button mapping only in a disposable source copy.
    $needle='return HTMAXBUTTON;'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected one HTMAXBUTTON mapping'}
    $mutation=$original.Replace($needle,'return HTCLOSE;')
    $mutation=$mutation.Replace('#include "../stdafx.h"','#include "'+(Join-Path $RepoRoot 'TrafficMonitor/stdafx.h').Replace('\','/')+'"')
    $mutantSource=Join-Path $work 'TitleBar.cpp'
    [IO.File]::WriteAllText($mutantSource,$mutation)
    $mutant=Build-Test $mutantSource 'MutatedTitleBarTests'
    $output=& $mutant
    $result=$LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if($result -ne 1 -or ($output -join "`n") -notmatch 'FAIL maximize button reports HTMAXBUTTON'){throw 'HTMAXBUTTON mapping mutation was not rejected'}
    if([IO.File]::ReadAllText($source) -cne $original){throw 'Production source changed during mutation verification'}
    Write-Host 'Expected rejection: disposable HTMAXBUTTON mapping mutation returned 1.'
    & $exe | Out-Null
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }
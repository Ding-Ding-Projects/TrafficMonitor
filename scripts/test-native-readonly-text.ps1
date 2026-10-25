$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-readonly-text-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    function Build-Test($controls,$name) {
        $exe=Join-Path $work ($name+'.exe')
        $sources=@('ReadOnlyTextTests.cpp','ButtonVisual.cpp','Theme.cpp','LayoutObservations.cpp') | ForEach-Object {'"'+(Join-Path $RepoRoot ('TrafficMonitor/NativeMaterial/'+$_))+'"'}
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I"'+(Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial')+'" '+($sources -join ' ')+' "'+$controls+'" /Fe:"'+$exe+'" /link UIAutomationCore.lib'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
        if($compileExit -ne 0){throw "Compilation returned $compileExit"}
        return $exe
    }
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial/Controls.cpp'
    $original=[IO.File]::ReadAllText($source)
    $exe=Build-Test $source 'ReadOnlyTextTests'
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Restore the former Tab ownership behavior only in a disposable source copy.
    $needle='if(tab)return result&~(DLGC_WANTALLKEYS|DLGC_WANTTAB);'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected one Tab dialog-code condition'}
    $mutation=$original.Replace($needle,'if(tab)return result;')
    $mutation=$mutation.Replace('#include "../stdafx.h"','#include "'+(Join-Path $RepoRoot 'TrafficMonitor/stdafx.h').Replace('\','/')+'"')
    $mutantSource=Join-Path $work 'Controls.cpp'
    [IO.File]::WriteAllText($mutantSource,$mutation)
    $mutant=Build-Test $mutantSource 'MutatedReadOnlyTextTests'
    $output=& $mutant
    $result=$LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if($result -ne 1 -or ($output -join "`n") -notmatch 'FAIL Tab with valid MSG releases dialog navigation'){throw 'Preview Tab ownership mutation was not rejected'}
    if([IO.File]::ReadAllText($source) -cne $original){throw 'Production source changed during mutation verification'}
    Write-Host 'Expected rejection: disposable preview Tab ownership mutation returned 1.'
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

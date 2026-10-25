$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-result-list-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    function Build-Test($navigation,$name) {
        $exe=Join-Path $work ($name+'.exe')
        $sources=@('ResultListTests.cpp','ScrollRail.cpp','Theme.cpp','LayoutObservations.cpp') | ForEach-Object {'"'+(Join-Path $RepoRoot ('TrafficMonitor/NativeMaterial/'+$_))+'"'}
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I"'+(Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial')+'" '+($sources -join ' ')+' "'+$navigation+'" /Fe:"'+$exe+'" /link UIAutomationCore.lib'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
        if($compileExit -ne 0){throw "Compilation returned $compileExit"}
        New-Item -ItemType File -Path ($exe+'.ui-diagnostics') | Out-Null
        return $exe
    }
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial/Navigation.cpp'
    $original=[IO.File]::ReadAllText($source)
    $exe=Build-Test $source 'ResultListTests'
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Restore the former all-row insertion loop only in a disposable source copy.
    $needle='if(insertion&&result>=0)RefreshMeasurements(static_cast<int>(result));'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected one insertion measurement condition'}
    $mutation=$original.Replace($needle,'if(insertion&&result>=0){for(int i=0;i<GetCount();++i)MeasureRow(i);}')
    $mutation=$mutation.Replace('#include "../stdafx.h"','#include "'+(Join-Path $RepoRoot 'TrafficMonitor/stdafx.h').Replace('\','/')+'"')
    $mutation=$mutation.Replace('#include "../NativeMaterial/ResponsivenessDiagnostics.h"','#include "ResponsivenessDiagnostics.h"')
    $mutantSource=Join-Path $work 'Navigation.cpp'
    [IO.File]::WriteAllText($mutantSource,$mutation)
    $mutant=Build-Test $mutantSource 'MutatedResultListTests'
    $output=& $mutant
    $result=$LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if($result -ne 1 -or ($output -join "`n") -notmatch 'FAIL append measurement is linear'){throw 'Quadratic insertion mutation was not rejected'}
    if([IO.File]::ReadAllText($source) -cne $original){throw 'Production source changed during mutation verification'}
    Write-Host 'Expected rejection: disposable quadratic insertion mutation returned 1.'
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

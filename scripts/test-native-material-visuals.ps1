$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-material-visuals-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    function Build-Test($visual,$name) {
        $exe=Join-Path $work ($name+'.exe')
        $sources=@('ButtonVisualTests.cpp','Theme.cpp','LayoutObservations.cpp') | ForEach-Object {'"'+(Join-Path $RepoRoot ('TrafficMonitor/NativeMaterial/'+$_))+'"'}
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I"'+(Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial')+'" '+($sources -join ' ')+' "'+$visual+'" /Fe:"'+$exe+'" /link UIAutomationCore.lib'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
        if($compileExit -ne 0){throw "Compilation returned $compileExit"}
        return $exe
    }
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial/ButtonVisual.cpp'
    $original=[IO.File]::ReadAllText($source)
    $exe=Build-Test $source 'ButtonVisualTests'
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Restore the former missing selector outline only in a disposable source copy.
    $needle='if(variant==ButtonVariant::Outlined||selector||c.highContrast)'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected one selector outline condition'}
    $mutation=$original.Replace($needle,'if(variant==ButtonVariant::Outlined||c.highContrast)')
    $mutation=$mutation.Replace('#include "../stdafx.h"','#include "'+(Join-Path $RepoRoot 'TrafficMonitor/stdafx.h').Replace('\','/')+'"')
    $mutantSource=Join-Path $work 'ButtonVisual.cpp'
    [IO.File]::WriteAllText($mutantSource,$mutation)
    $mutant=Build-Test $mutantSource 'MutatedButtonVisualTests'
    $output=& $mutant
    $result=$LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if($result -ne 1 -or ($output -join "`n") -notmatch 'FAIL selector has visible outline'){throw 'Selector outline mutation was not rejected'}
    if([IO.File]::ReadAllText($source) -cne $original){throw 'Production source changed during mutation verification'}
    Write-Host 'Expected rejection: disposable selector outline mutation returned 1.'
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

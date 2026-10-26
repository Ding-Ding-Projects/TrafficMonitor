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
    # Material buttons draw text through DirectWrite with no mnemonic processing, so a lone '&' in a button
    # label would be painted literally and leak into the accessible name. Fail on any such label literal.
    function Get-StrayAmpersandButtonLabels {
        $bad=@()
        foreach($file in Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'TrafficMonitor') -Recurse -File -Include *.cpp,*.h) {
            $lines=[IO.File]::ReadAllLines($file.FullName)
            $names=@()
            foreach($line in $lines){ if($line -match '\bButton\s+([A-Za-z_][\w\s,]*);'){ $names+=($Matches[1] -split '[,\s]+' | Where-Object {$_}) } }
            foreach($name in ($names | Select-Object -Unique)) {
                $pattern='\b'+[regex]::Escape($name)+'\s*\.\s*(Create|SetLabel)\s*\('
                for($i=0;$i -lt $lines.Count;$i++){
                    if($lines[$i] -match $pattern){
                        foreach($m in [regex]::Matches($lines[$i],'L"((?:[^"\\]|\\.)*)"')){
                            if($m.Groups[1].Value -match '(?<!&)&(?!&)'){ $bad+=("{0}:{1}: {2}" -f $file.Name,($i+1),$m.Groups[1].Value) }
                        }
                    }
                }
            }
        }
        return $bad
    }
    $badLabels=@(Get-StrayAmpersandButtonLabels)
    if($badLabels.Count -gt 0){ $badLabels | ForEach-Object { Write-Host "FAIL Material button label has stray ampersand: $_" }; throw 'Material button labels must not contain mnemonic ampersands' }
    Write-Host 'PASS no Material button label literal contains a stray ampersand'
    # Wrapped text must fit the height measured for it, and scrollable content must reach every placed control.
    $extentDir=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial'
    function Build-ExtentTest($directory,$name) {
        $exe=Join-Path $work ($name+'.exe')
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /DUNICODE /D_UNICODE "'+(Join-Path $directory 'TextExtentTests.cpp')+'" /Fe:"'+$exe+'" /link user32.lib gdi32.lib'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; $exit=$LASTEXITCODE } finally { Pop-Location }
        if($exit -ne 0){throw "Text extent compilation returned $exit"}
        return $exe
    }
    $extentExe=Build-ExtentTest $extentDir 'TextExtentTests'
    & $extentExe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    $header=[IO.File]::ReadAllText((Join-Path $extentDir 'TextExtent.h'))
    $extentMutations=@(
        @{ name='measure without word wrapping'; needle='DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS | DT_EDITCONTROL'; replacement='DT_CALCRECT | DT_NOPREFIX | DT_EXPANDTABS | DT_EDITCONTROL'; expected='FAIL updater paragraph fits' },
        @{ name='measure without breaking over-long words'; needle='DT_NOPREFIX | DT_EXPANDTABS | DT_EDITCONTROL'; replacement='DT_NOPREFIX | DT_EXPANDTABS'; expected='FAIL updater paragraph fits' },
        @{ name='content extent ignores placed controls'; needle='covered = (std::max)(covered, bottom);'; replacement='(void)bottom;'; expected='FAIL content extent grows' }
    )
    foreach($mutation in $extentMutations) {
        if(($header.Split([string[]]@($mutation.needle),[StringSplitOptions]::None)).Count -ne 2){throw "Expected one site for the $($mutation.name) mutation"}
        $mutantDir=Join-Path $work ('extent-'+[guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $mutantDir | Out-Null
        [IO.File]::WriteAllText((Join-Path $mutantDir 'TextExtent.h'),$header.Replace($mutation.needle,$mutation.replacement))
        Copy-Item -LiteralPath (Join-Path $extentDir 'TextExtentTests.cpp') -Destination (Join-Path $mutantDir 'TextExtentTests.cpp')
        $mutantExe=Build-ExtentTest $mutantDir ('Mutated'+[guid]::NewGuid().ToString('N'))
        $mutantOutput=& $mutantExe
        $mutantResult=$LASTEXITCODE
        $mutantOutput | ForEach-Object { Write-Host $_ }
        if($mutantResult -ne 1 -or ($mutantOutput -join "`n") -notmatch $mutation.expected){throw "The $($mutation.name) mutation was not rejected"}
        Write-Host "Expected rejection: $($mutation.name) returned 1."
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

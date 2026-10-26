$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
# Legacy message boxes were converted to the Material 3 alert dialog. None may return in these files.
$converted=@('TrafficMonitor.cpp','TrafficMonitorDlg.cpp','GeneralSettingsDlg.cpp','MainWndSettingsDlg.cpp','TaskBarSettingsDlg.cpp','NetworkInfoDlg.cpp','PluginInfoDlg.cpp','PluginManagerDlg.cpp','SelectConnectionsDlg.cpp','TaskBarDlg.cpp','TaskBarDlgDrawCommon.cpp')
function Find-SystemMessageBoxes($paths) {
    $found=@()
    foreach($path in $paths) {
        $number=0
        foreach($line in [IO.File]::ReadAllLines($path)) {
            $number++
            if($line.TrimStart().StartsWith('//')){continue}
            if($line -match '\bAfxMessageBox\s*\(|\bMessageBox[AW]?\s*\('){ $found+=('{0}:{1}: {2}' -f $path,$number,$line.Trim()) }
        }
    }
    return $found
}
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-alert-dialog-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $app=Join-Path $RepoRoot 'TrafficMonitor'
    # 1. Source scan, with a self-check that the scan really goes red on a system message box.
    $probe=Join-Path $work 'probe.cpp'
    [IO.File]::WriteAllText($probe,'void f(){ AfxMessageBox(L"x"); }')
    if((Find-SystemMessageBoxes @($probe)).Count -ne 1){throw 'Source scan did not detect a planted AfxMessageBox call'}
    [IO.File]::WriteAllText($probe,'void f(){ ::MessageBox(NULL,L"x",NULL,MB_OK); }')
    if((Find-SystemMessageBoxes @($probe)).Count -ne 1){throw 'Source scan did not detect a planted MessageBox call'}
    $hits=Find-SystemMessageBoxes ($converted | ForEach-Object { Join-Path $app $_ })
    if($hits.Count){ $hits | ForEach-Object { Write-Host "FAIL system message box: $_" }; throw 'System message boxes returned in converted files' }
    Write-Host ('PASS no AfxMessageBox/MessageBox calls remain in the '+$converted.Count+' converted files')
    # 2. Focused dialog program, then one disposable mutation of a rule.
    function Build-Test($alert,$name) {
        $exe=Join-Path $work ($name+'.exe')
        $dir=Join-Path $app 'NativeMaterial'
        $sources=@((Join-Path $dir 'AlertDialogTests.cpp'),$alert,(Join-Path $dir 'Controls.cpp'),(Join-Path $dir 'ButtonVisual.cpp'),(Join-Path $dir 'Theme.cpp'),(Join-Path $dir 'LayoutObservations.cpp'),(Join-Path $app 'NativePreferences/Text.cpp')) | ForEach-Object {'"'+$_+'"'}
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I"'+$dir+'" '+($sources -join ' ')+' /Fe:"'+$exe+'" /link UIAutomationCore.lib'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; $compileExit=$LASTEXITCODE } finally { Pop-Location }
        if($compileExit -ne 0){throw "Compilation returned $compileExit"}
        return $exe
    }
    $source=Join-Path $app 'NativeMaterial/AlertDialog.cpp'
    $original=[IO.File]::ReadAllText($source)
    $exe=Build-Test $source 'AlertDialogTests'
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Restore a wrong default-button mapping only in a disposable source copy.
    $needle='case MB_DEFBUTTON2: spec.defaultIndex = 1; break;'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected one MB_DEFBUTTON2 mapping'}
    $mutation=$original.Replace($needle,'case MB_DEFBUTTON2: spec.defaultIndex = 0; break;')
    $mutation=$mutation.Replace('#include "../','#include "'+$app.Replace('\','/')+'/')
    $mutantSource=Join-Path $work 'AlertDialog.cpp'
    [IO.File]::WriteAllText($mutantSource,$mutation)
    $mutant=Build-Test $mutantSource 'MutatedAlertDialogTests'
    $output=& $mutant
    $result=$LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if($result -ne 1 -or ($output -join "`n") -notmatch 'FAIL MB_YESNO\|MB_DEFBUTTON2 focuses No'){throw 'Default-button mutation was not rejected'}
    if([IO.File]::ReadAllText($source) -cne $original){throw 'Production source changed during mutation verification'}
    Write-Host 'Expected rejection: disposable MB_DEFBUTTON2 mapping mutation returned 1.'
    & $exe | Select-Object -Last 1
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }
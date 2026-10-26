$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-material-pickers-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $materialDir=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial'
    $include='"'+$materialDir+'"'
    $common='/nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I'+$include
    # The focused test links the three pickers and what they use (the input components, the list and its data table,
    # the Material buttons). The application searchable menu and the preference service are stubbed by the test itself.
    $sourcePaths=@(
        'TrafficMonitor/NativeMaterial/PickerTests.cpp','TrafficMonitor/NativeMaterial/PickerDialog.cpp','TrafficMonitor/NativeMaterial/ColorPicker.cpp',
        'TrafficMonitor/NativeMaterial/FontPicker.cpp','TrafficMonitor/NativeMaterial/Calendar.cpp',
        'TrafficMonitor/NativeMaterial/InputWindow.cpp','TrafficMonitor/NativeMaterial/InputToggles.cpp','TrafficMonitor/NativeMaterial/InputSelection.cpp',
        'TrafficMonitor/NativeMaterial/NumberStepper.cpp','TrafficMonitor/NativeMaterial/Dropdown.cpp','TrafficMonitor/NativeMaterial/LabeledTextField.cpp',
        'TrafficMonitor/NativeMaterial/InputVisuals.cpp','TrafficMonitor/NativeMaterial/TextField.cpp','TrafficMonitor/NativeMaterial/Slider.cpp',
        'TrafficMonitor/NativeMaterial/DataTable.cpp','TrafficMonitor/NativeMaterial/MaterialList.cpp','TrafficMonitor/NativeMaterial/TableVisuals.cpp',
        'TrafficMonitor/NativeMaterial/ScrollRail.cpp','TrafficMonitor/NativeMaterial/Controls.cpp','TrafficMonitor/NativeMaterial/ButtonVisual.cpp',
        'TrafficMonitor/NativeMaterial/Theme.cpp','TrafficMonitor/NativeMaterial/LayoutObservations.cpp','TrafficMonitor/NativePreferences/Text.cpp')
    function Invoke-Cmd($command){
        $log=Join-Path $work 'step.log'
        & cmd.exe /d /c ($command+' > "'+$log+'" 2>&1')
        $exit=$LASTEXITCODE
        # The pre-existing target version header is not UTF-8; its encoding warning is not a finding of these sources.
        Get-Content -LiteralPath $log | Where-Object { $_ -notmatch 'C4828|vswhere|^operable program' } | ForEach-Object { Write-Host $_ }
        return $exit
    }
    # Compiles the sources (or only the replaced ones when a base object folder is given) and links the test.
    function Build-Test($directory,$replacements,$baseObjects) {
        $objects=Join-Path $directory 'obj'
        New-Item -ItemType Directory -Path $objects -Force | Out-Null
        $inputs=@()
        foreach($relative in $sourcePaths){
            if($baseObjects -and -not $replacements.ContainsKey($relative)){ continue }
            $path=Join-Path $RepoRoot $relative
            if($replacements.ContainsKey($relative)){ $path=$replacements[$relative] }
            $inputs+=('"'+$path+'"')
        }
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" '+$common+' /MP /c '+($inputs -join ' ')+' /Fo"'+$objects+'\\"'
        $exit=Invoke-Cmd $command
        if($exit -ne 0){throw "Compilation returned $exit"}
        $objectList=(($sourcePaths | ForEach-Object {
            $name=[IO.Path]::GetFileNameWithoutExtension($_)+'.obj'
            $folder=$objects
            if($baseObjects -and -not $replacements.ContainsKey($_)){ $folder=$baseObjects }
            '"'+(Join-Path $folder $name)+'"' }) -join ' ')
        $exe=Join-Path $directory 'MaterialPickerTests.exe'
        $linkCommand='call "'+$vcvars+'" >nul && link /nologo /OUT:"'+$exe+'" '+$objectList+' UIAutomationCore.lib oleacc.lib'
        $exit=Invoke-Cmd $linkCommand
        if($exit -ne 0){throw "Link returned $exit"}
        return $exe
    }
    $exe=Build-Test $work @{} $null
    $output=& $exe
    $result=$LASTEXITCODE
    $output | Where-Object { $_ -notlike 'PASS *' } | ForEach-Object { Write-Host $_ }
    if($result -ne 0){ Write-Host "Material picker verification failed ($result)."; exit $result }
    Write-Host ("Material picker checks passed: {0}" -f (@($output | Where-Object { $_ -like 'PASS *' }).Count))

    # Source scan: the pickers draw everything themselves. None of them may name a common dialog, a stock control,
    # a month calendar control or a system message box.
    function Test-NoStockPickerControls($sources) {
        $failures=@()
        foreach($name in $sources.Keys){
            # Comments may name the controls the pickers replace.
            $code=[regex]::Replace($sources[$name],'//[^\r\n]*','')
            foreach($pattern in @('\bCFontDialog\b','\bCColorDialog\b','\bCMFCColorDialog\w*','\bCMonthCalCtrl\b','\bCDateTimeCtrl\b','\bCComboBox\b','\bCSliderCtrl\b','\bCSpinButtonCtrl\b','\bCListCtrl\b','\bCStatic\b','\bCButton\b','\bMessageBox\w*','\bChooseFont\b','\bChooseColor\b','MONTHCAL_CLASS','msctls_trackbar32','\bTBS_')){
                if($code -match $pattern){ $failures+=("{0} uses {1}" -f $name,$pattern) }
            }
        }
        foreach($required in @('ColorPicker.cpp','FontPicker.cpp','Calendar.cpp','PickerDialog.cpp')){
            if(-not $sources.ContainsKey($required)){ $failures+="$required is missing" }
        }
        return ,$failures
    }
    $pickerSources=@{}
    foreach($name in 'PickerDialog.cpp','PickerDialog.h','ColorPicker.cpp','ColorPicker.h','FontPicker.cpp','FontPicker.h','Calendar.cpp','Calendar.h','Calendar.Accessibility.h','PickerDraw.h'){
        $pickerSources[$name]=[IO.File]::ReadAllText((Join-Path $materialDir $name))
    }
    $scan=Test-NoStockPickerControls $pickerSources
    if($scan.Count -gt 0){ $scan | ForEach-Object { Write-Host "FAIL $_" }; throw 'A picker names a common dialog or a stock control.' }
    Write-Host 'PASS the pickers name no common dialog, stock control, month calendar control or system message box'
    foreach($probe in @(@{file='FontPicker.cpp';text='CFontDialog probe;'},@{file='ColorPicker.cpp';text='CMFCColorDialog probe;'},@{file='Calendar.cpp';text='CMonthCalCtrl probe;'},@{file='PickerDialog.cpp';text='::MessageBoxW(nullptr,L"x",L"y",MB_OK);'})){
        $copy=@{}; foreach($key in $pickerSources.Keys){ $copy[$key]=$pickerSources[$key] }
        $copy[$probe.file]=$copy[$probe.file]+"`n"+$probe.text
        if((Test-NoStockPickerControls $copy).Count -eq 0){ throw "The stock control scan did not reject: $($probe.text)" }
        Write-Host "Expected rejection: stock control scan rejected '$($probe.text)'."
    }

    # Mutation checks: break one rule at a time in a disposable copy, expect the named check to turn red.
    # Copies live in the work folder, so the production tree is never edited.
    $mutations=@(
        @{ name='hex parser accepts any letter above F'; file='TrafficMonitor/NativeMaterial/ColorPicker.cpp';
           needle="            if (ch >= L'A' && ch <= L'F') return ch - L'A' + 10;"; replacement="            if (ch >= L'A') return 10;";
           expected='FAIL hex parser rejects a non-hex letter' },
        @{ name='every fourth year is a leap year'; file='TrafficMonitor/NativeMaterial/Calendar.cpp';
           needle='        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);'; replacement='        return year % 4 == 0;';
           expected='FAIL leap years follow the 400 year rule' }
    )
    foreach($mutation in $mutations) {
        $path=Join-Path $RepoRoot $mutation.file
        $original=[IO.File]::ReadAllText($path)
        $needle=$mutation.needle
        if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){
            # Sources may carry CRLF endings; retry the needle with CRLF.
            $needle=$mutation.needle.Replace("`r`n","`n").Replace("`n","`r`n"); $replacement=$mutation.replacement
            if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw "Expected exactly one site for the '$($mutation.name)' mutation"}
        } else { $replacement=$mutation.replacement }
        $mutantDir=Join-Path $work ('mutant-'+[guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $mutantDir | Out-Null
        $mutantSource=Join-Path $mutantDir ([IO.Path]::GetFileName($mutation.file))
        $mutated=$original.Replace($needle,$replacement).Replace('#include "../stdafx.h"','#include "'+(Join-Path $RepoRoot 'TrafficMonitor/stdafx.h').Replace('\','/')+'"')
        [IO.File]::WriteAllText($mutantSource,$mutated,(New-Object Text.UTF8Encoding($false)))
        $mutantExe=Build-Test $mutantDir @{ $mutation.file=$mutantSource } (Join-Path $work 'obj')
        $mutantOutput=& $mutantExe
        $mutantResult=$LASTEXITCODE
        $mutantOutput | Where-Object { $_ -like 'FAIL *' } | ForEach-Object { Write-Host $_ }
        if($mutantResult -ne 1 -or ($mutantOutput -join "`n") -notmatch [regex]::Escape($mutation.expected)){throw "The '$($mutation.name)' mutation was not rejected by the expected check"}
        if([IO.File]::ReadAllText($path) -cne $original){throw 'Production source changed during mutation verification'}
        Write-Host "Expected rejection: $($mutation.name) returned 1."
    }
    $final=& $exe
    $finalResult=$LASTEXITCODE
    if($finalResult -ne 0){throw 'The unmodified picker verification no longer passes after the mutations.'}
    Write-Host 'Material picker verification passed, including mutation rejection.'
    exit 0
} catch { Write-Error $_; exit 1 }

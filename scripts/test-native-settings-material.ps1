$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
# Verifies, in a never-shown window, that the native Settings rows and the embedded Interface preferences
# editor contain Material components only (no stock BUTTON, EDIT, COMBOBOX, LISTBOX or common control, except
# the entry field owned by a NumberStepper) and that apply, revert and refusal still work through them.
# Mutation checks break one rule at a time in a disposable copy and expect the matching check to turn red.
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-settings-material-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $app=Join-Path $RepoRoot 'TrafficMonitor'
    $sourcePaths=@(
        'NativeSettings/SettingsMaterialTests.cpp','NativeSettings/RowControls.cpp','NativeMaterial/SettingsControls.cpp',
        'NativeMaterial/InputWindow.cpp','NativeMaterial/InputToggles.cpp','NativeMaterial/InputSelection.cpp','NativeMaterial/NumberStepper.cpp',
        'NativeMaterial/Dropdown.cpp','NativeMaterial/LabeledTextField.cpp','NativeMaterial/InputVisuals.cpp','NativeMaterial/TextField.cpp',
        'NativeMaterial/Theme.cpp','NativeMaterial/LayoutObservations.cpp','NativeMaterial/Controls.cpp','NativeMaterial/ButtonVisual.cpp',
        'NativeMaterial/ButtonDrawTextBaseline.cpp','NativeMaterial/Panel.cpp','NativeMaterial/ActionSounds.cpp',
        'NativePreferences/Editor.cpp','NativePreferences/Text.cpp','NativePreferences/Preferences.cpp','NativePreferences/Storage.cpp',
        'NativePreferences/Subscription.cpp','IniHelper.cpp','CommonText.cpp')
    $includes=' /I"'+(Join-Path $RepoRoot 'include')+'" /I"'+(Join-Path $app 'NativeMaterial')+'" /I"'+(Join-Path $app 'NativeSettings')+'" /I"'+(Join-Path $app 'NativePreferences')+'"'
    $common='/nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /DWITHOUT_TEMPERATURE'+$includes
    function Invoke-Cmd($command){
        $log=Join-Path $work 'step.log'
        & cmd.exe /d /c ($command+' > "'+$log+'" 2>&1')
        $exit=$LASTEXITCODE
        # The pre-existing target version header is not UTF-8 and vswhere may be absent; neither is a finding of these sources.
        Get-Content -LiteralPath $log | Where-Object { $_ -notmatch 'C4828|vswhere|^operable program' } | ForEach-Object { Write-Host $_ }
        return $exit
    }
    # Compiles every source (or only the replaced ones when a base object folder is given) and links the test.
    function Build-Test($directory,$replacements,$baseObjects) {
        $objects=Join-Path $directory 'obj'
        New-Item -ItemType Directory -Path $objects -Force | Out-Null
        $inputs=@()
        foreach($relative in $sourcePaths){
            if($baseObjects -and -not $replacements.ContainsKey($relative)){ continue }
            $path=Join-Path $app $relative
            if($replacements.ContainsKey($relative)){ $path=$replacements[$relative] }
            $inputs+=('"'+$path+'"')
        }
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" '+$common+' /MP /c '+($inputs -join ' ')+' /Fo"'+$objects+'\\"'
        if((Invoke-Cmd $command) -ne 0){throw 'Compilation failed'}
        $objectList=(($sourcePaths | ForEach-Object {
            $name=[IO.Path]::GetFileNameWithoutExtension($_)+'.obj'
            $folder=$objects
            if($baseObjects -and -not $replacements.ContainsKey($_)){ $folder=$baseObjects }
            '"'+(Join-Path $folder $name)+'"' }) -join ' ')
        $exe=Join-Path $directory 'SettingsMaterialTests.exe'
        $linkCommand='call "'+$vcvars+'" >nul && link /nologo /OUT:"'+$exe+'" '+$objectList+' UIAutomationCore.lib oleacc.lib'
        if((Invoke-Cmd $linkCommand) -ne 0){throw 'Link failed'}
        return $exe
    }
    $exe=Build-Test $work @{} $null
    $output=& $exe
    $result=$LASTEXITCODE
    $output | Where-Object { $_ -notlike 'PASS *' } | ForEach-Object { Write-Host $_ }
    if($result -ne 0){ Write-Host "Settings Material verification failed ($result)."; exit $result }
    Write-Host ("Settings Material checks passed: {0}" -f (@($output | Where-Object { $_ -like 'PASS *' }).Count))

    $mutations=@(
        @{ name='the audit stops treating BUTTON as a stock class'; file='NativeMaterial/SettingsControls.cpp';
           needle='{L"Button",L"Edit"'; replacement='{L"Edit"';
           expected='FAIL audit reports a stray BUTTON, a loose EDIT and a COMBOBOX' },
        @{ name='a value off the step grid is accepted into the draft'; file='NativeSettings/RowControls.cpp';
           needle='if (!ParseNumber(std::to_wstring(value), shape.minimum, shape.maximum, shape.step, accepted))'; replacement='if (accepted != 0)';
           expected='FAIL a value off the 100 step is refused and the control returns to the draft value' },
        @{ name='a segmented selection no longer reaches the interface draft'; file='NativePreferences/Editor.cpp';
           needle='if(code!=CBN_SELCHANGE)return true;'; replacement='return true;';
           expected='FAIL choosing the third appearance segment changes the draft and marks it unsaved' }
    )
    foreach($mutation in $mutations) {
        $path=Join-Path $app $mutation.file
        $original=[IO.File]::ReadAllText($path)
        if(($original.Split([string[]]@($mutation.needle),[StringSplitOptions]::None)).Count -ne 2){throw "Expected exactly one site for the '$($mutation.name)' mutation"}
        $mutantDir=Join-Path $work ('mutant-'+[guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $mutantDir | Out-Null
        $mutantSource=Join-Path $mutantDir ([IO.Path]::GetFileName($mutation.file))
        $absolute='#include "'+$app.Replace('\','/')+'/'
        $mutated=$original.Replace($mutation.needle,$mutation.replacement).Replace('#include "../',$absolute)
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
    if($LASTEXITCODE -ne 0){throw 'The unmodified Settings Material verification no longer passes after the mutations.'}
    Write-Host 'Settings Material verification passed, including mutation rejection.'
    exit 0
} catch { Write-Error $_; exit 1 }

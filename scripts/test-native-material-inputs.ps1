$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-material-inputs-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $materialDir=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial'
    $include='"'+$materialDir+'"'
    $common='/nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I'+$include
    # The focused test links only the input components and what they use. The application searchable menu and
    # the preference service are stubbed by the test itself.
    $sourcePaths=@(
        'TrafficMonitor/NativeMaterial/InputTests.cpp','TrafficMonitor/NativeMaterial/InputWindow.cpp','TrafficMonitor/NativeMaterial/InputToggles.cpp',
        'TrafficMonitor/NativeMaterial/InputSelection.cpp','TrafficMonitor/NativeMaterial/NumberStepper.cpp','TrafficMonitor/NativeMaterial/Dropdown.cpp',
        'TrafficMonitor/NativeMaterial/LabeledTextField.cpp','TrafficMonitor/NativeMaterial/InputVisuals.cpp','TrafficMonitor/NativeMaterial/TextField.cpp',
        'TrafficMonitor/NativeMaterial/Slider.cpp','TrafficMonitor/NativeMaterial/Theme.cpp','TrafficMonitor/NativeMaterial/LayoutObservations.cpp','TrafficMonitor/NativePreferences/Text.cpp')
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
        $exe=Join-Path $directory 'MaterialInputTests.exe'
        $linkCommand='call "'+$vcvars+'" >nul && link /nologo /OUT:"'+$exe+'" '+$objectList+' UIAutomationCore.lib oleacc.lib'
        $exit=Invoke-Cmd $linkCommand
        if($exit -ne 0){throw "Link returned $exit"}
        return $exe
    }    $exe=Build-Test $work @{} $null
    $output=& $exe
    $result=$LASTEXITCODE
    $output | Where-Object { $_ -notlike 'PASS *' } | ForEach-Object { Write-Host $_ }
    if($result -ne 0){ Write-Host "Material input verification failed ($result)."; exit $result }
    Write-Host ("Material input checks passed: {0}" -f (@($output | Where-Object { $_ -like 'PASS *' }).Count))

    # Source scan: the setup wizard and the workspace build no stock combo box, trackbar, static text or system message box.
    # The wizard needs the application and a desktop session, so it cannot be created in this focused program; the
    # window classes it can create are exactly the classes its sources name.
    function Test-NoStockDashboardControls($sources) {
        $failures=@()
        foreach($name in $sources.Keys){
            foreach($pattern in @('NativeMaterial::Choice\b','NativeMaterial::RangeValue\b','\bCComboBox\b','\bCSliderCtrl\b','\bCStatic\b','\bMessageBox\b','\bCBS_DROPDOWN','msctls_trackbar32','\bTBS_')){
                if($sources[$name] -match $pattern){ $failures+=("{0} still uses {1}" -f $name,$pattern) }
            }
        }
        if($sources['SetupFlow.cpp'] -notmatch 'NativeMaterial::Dropdown' -or $sources['SetupFlow.cpp'] -notmatch 'NativeMaterial::Slider' -or $sources['SetupFlow.cpp'] -notmatch 'NativeMaterial::Label'){ $failures+='SetupFlow.cpp does not use Dropdown, Slider and Label' }
        if($sources['Workspace.cpp'] -notmatch 'NativeMaterial::Alert\(' -or $sources['Workspace.cpp'] -notmatch 'NativeMaterial::Label'){ $failures+='Workspace.cpp does not use Alert and Label' }
        return ,$failures
    }
    $dashboardSources=@{}
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'TrafficMonitor/NativeDashboard') -Include *.cpp,*.h -Recurse){ $dashboardSources[$file.Name]=[IO.File]::ReadAllText($file.FullName) }
    $scan=Test-NoStockDashboardControls $dashboardSources
    if($scan.Count -gt 0){ $scan | ForEach-Object { Write-Host "FAIL $_" }; throw 'The setup wizard or workspace still uses a stock control.' }
    Write-Host 'PASS setup wizard and workspace use no stock combo box, trackbar, static text or system message box'
    foreach($probe in @(@{file='SetupFlow.cpp';text='NativeMaterial::Choice m_probe;'},@{file='SetupFlow.cpp';text='CSliderCtrl m_probe;'},@{file='Workspace.cpp';text='CStatic m_probe;'},@{file='Workspace.cpp';text='owner->MessageBox(L"x",L"y",MB_OK);'})){
        $copy=@{}; foreach($key in $dashboardSources.Keys){ $copy[$key]=$dashboardSources[$key] }
        $copy[$probe.file]=$copy[$probe.file]+"`n"+$probe.text
        if((Test-NoStockDashboardControls $copy).Count -eq 0){ throw "The stock control scan did not reject: $($probe.text)" }
        Write-Host "Expected rejection: stock control scan rejected '$($probe.text)'."
    }

    # Mutation checks: break one rule at a time in a disposable copy, expect the named check to turn red.
    # Copies live in the work folder, so the production tree is never edited.
    $mutations=@(
        @{ name='slider no longer clamps at the maximum'; file='TrafficMonitor/NativeMaterial/Slider.cpp';
           needle='        value = Clamp(value, m_minimum, m_maximum);'; replacement='';
           expected='FAIL slider Right arrow at the maximum keeps the value and notifies nothing' },
        @{ name='slider stops notifying its parent'; file='TrafficMonitor/NativeMaterial/Slider.cpp';
           needle="        Notify(EN_CHANGE);`r`n        NotifyScroll(scrollCode);"; replacement="        NotifyScroll(scrollCode);";
           expected='FAIL slider Right arrow adds a step and notifies' },
        @{ name='stepper no longer clamps to the maximum'; file='TrafficMonitor/NativeMaterial/NumberStepper.cpp';
           needle='(std::max)(static_cast<long long>(m_minimum), (std::min)(static_cast<long long>(m_maximum), wanted))'; replacement='wanted';
           expected='FAIL stepping clamps at the maximum' },
        @{ name='switch stops notifying its parent'; file='TrafficMonitor/NativeMaterial/InputToggles.cpp';
           needle="        m_checked = !m_checked;`r`n        Publish();`r`n        Invalidate(FALSE);`r`n        Notify(BN_CLICKED);"; replacement="        m_checked = !m_checked;`r`n        Publish();`r`n        Invalidate(FALSE);";
           expected='FAIL switch click turns it on and notifies the parent once' },
        @{ name='dropdown filter lets every item through'; file='TrafficMonitor/NativeMaterial/Dropdown.cpp';
           needle='if (hay.Find(needle) >= 0) rows.push_back'; replacement='if (hay.Find(needle) >= -1) rows.push_back';
           expected='FAIL plain filter is case insensitive and narrows the list' },
        @{ name='radio arrow keys stop wrapping'; file='TrafficMonitor/NativeMaterial/InputToggles.cpp';
           needle='auto* next = group[(index + step * hop % count + count) % count];'; replacement='auto* next = group[(std::max)(0, (std::min)(count - 1, index + step * hop))];';
           expected='FAIL arrow key wraps to the first radio' }
    )
    foreach($mutation in $mutations) {
        $path=Join-Path $RepoRoot $mutation.file
        $original=[IO.File]::ReadAllText($path)
        $needle=$mutation.needle
        if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){
            # Sources may carry LF endings; retry the needle with LF.
            $needle=$needle.Replace("`r`n","`n"); $replacement=$mutation.replacement.Replace("`r`n","`n")
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
    if($finalResult -ne 0){throw 'The unmodified input verification no longer passes after the mutations.'}
    Write-Host 'Material input verification passed, including mutation rejection.'
    exit 0
} catch { Write-Error $_; exit 1 }

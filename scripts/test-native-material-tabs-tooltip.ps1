$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-material-tabs-tooltip-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $materialDir=Join-Path $RepoRoot 'TrafficMonitor/NativeMaterial'
    $include='"'+$materialDir+'"'
    $common='/nologo /std:c++17 /EHsc /utf-8 /MD /D_AFXDLL /D_UNICODE /DUNICODE /I'+$include
    # The focused test links only the tab bar, the tooltip and what they use. It runs on a private desktop of its own,
    # so the tooltip popups and the shown test windows never reach a user's screen.
    $sourcePaths=@(
        'TrafficMonitor/NativeMaterial/TabsTooltipTests.cpp','TrafficMonitor/NativeMaterial/Tabs.cpp','TrafficMonitor/NativeMaterial/Tooltip.cpp',
        'TrafficMonitor/NativeMaterial/InputWindow.cpp','TrafficMonitor/NativeMaterial/Theme.cpp','TrafficMonitor/NativeMaterial/LayoutObservations.cpp',
        'TrafficMonitor/NativePreferences/Text.cpp')
    function Invoke-Cmd($command){
        $log=Join-Path $work 'step.log'
        & cmd.exe /d /c ($command+' > "'+$log+'" 2>&1')
        $exit=$LASTEXITCODE
        # The pre-existing target version header is not UTF-8; its encoding warning is not a finding of these sources.
        Get-Content -LiteralPath $log | Where-Object { $_ -notmatch 'C4828|vswhere|^operable program' } | ForEach-Object { Write-Host $_ }
        return $exit
    }
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
        $exe=Join-Path $directory 'MaterialTabsTooltipTests.exe'
        $linkCommand='call "'+$vcvars+'" >nul && link /nologo /OUT:"'+$exe+'" '+$objectList+' UIAutomationCore.lib oleacc.lib comctl32.lib'
        $exit=Invoke-Cmd $linkCommand
        if($exit -ne 0){throw "Link returned $exit"}
        return $exe
    }
    $exe=Build-Test $work @{} $null
    $output=& $exe
    $result=$LASTEXITCODE
    $output | Where-Object { $_ -notlike 'PASS *' } | ForEach-Object { Write-Host $_ }
    if($result -ne 0){ Write-Host "Material tabs and tooltip verification failed ($result)."; exit $result }
    Write-Host ("Material tabs and tooltip checks passed: {0}" -f (@($output | Where-Object { $_ -like 'PASS *' }).Count))

    # Source scan: the new components create no stock tab control and no stock tooltip control.
    function Test-NoStockTabsOrTooltips($sources) {
        $failures=@()
        foreach($name in $sources.Keys){
            foreach($pattern in @('\bCTabCtrl\b','\bCToolTipCtrl\b','WC_TABCONTROL','TOOLTIPS_CLASS','SysTabControl32','tooltips_class32','\bTTM_','\bTCM_','\bCPropertySheet\b')){
                if($sources[$name] -match $pattern){ $failures+=("{0} uses {1}" -f $name,$pattern) }
            }
        }
        return ,$failures
    }
    $componentSources=@{}
    foreach($name in 'Tabs.cpp','Tooltip.cpp'){ $componentSources[$name]=[IO.File]::ReadAllText((Join-Path $materialDir $name)) }
    $scan=Test-NoStockTabsOrTooltips $componentSources
    if($scan.Count -gt 0){ $scan | ForEach-Object { Write-Host "FAIL $_" }; throw 'The tab bar or tooltip sources use a stock control.' }
    Write-Host 'PASS tab bar and tooltip sources use no stock tab or tooltip control'
    foreach($probe in @(@{file='Tabs.cpp';text='CTabCtrl m_probe;'},@{file='Tooltip.cpp';text='CToolTipCtrl m_probe;'},@{file='Tooltip.cpp';text='const wchar_t* probe = L"tooltips_class32";'})){
        $copy=@{}; foreach($key in $componentSources.Keys){ $copy[$key]=$componentSources[$key] }
        $copy[$probe.file]=$copy[$probe.file]+"`n"+$probe.text
        if((Test-NoStockTabsOrTooltips $copy).Count -eq 0){ throw "The stock control scan did not reject: $($probe.text)" }
        Write-Host "Expected rejection: stock control scan rejected '$($probe.text)'."
    }

    # Mutation checks: break one rule at a time in a disposable copy, expect the named check to turn red.
    $mutations=@(
        @{ name='tab selection stops notifying TCN_SELCHANGE'; file='TrafficMonitor/NativeMaterial/Tabs.cpp';
           needle="            header.code = TCN_SELCHANGE;`r`n            parent->SendMessage(WM_NOTIFY, header.idFrom, reinterpret_cast<LPARAM>(&header));"; replacement='            (void)parent;';
           expected='FAIL tab click selects, asks TCN_SELCHANGING and notifies TCN_SELCHANGE once' },
        @{ name='tooltip placement stops clamping into the work area'; file='TrafficMonitor/NativeMaterial/Tooltip.cpp';
           needle='LONG left = (std::max)(workArea.left, (std::min)(static_cast<LONG>(anchorLeft), workArea.right - size.cx));'; replacement='LONG left = anchorLeft;';
           expected='FAIL placement shifts left to stay in the work area' },
        @{ name='pointer leave stops hiding the tip'; file='TrafficMonitor/NativeMaterial/Tooltip.cpp';
           needle='if (m_shownTool == window->m_hWnd && !m_shownByKeyboard) Hide();'; replacement='(void)0;';
           expected='FAIL pointer leave hides the tip' },
        @{ name='reduced motion no longer skips the fade'; file='TrafficMonitor/NativeMaterial/Tooltip.cpp';
           needle='if (g_animationOverride == 0) return false;'; replacement='if (g_animationOverride == 0) return true;';
           expected='FAIL with animation off (reduced motion) the tip appears at full opacity at once' }
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
    if($finalResult -ne 0){throw 'The unmodified tabs and tooltip verification no longer passes after the mutations.'}
    Write-Host 'Material tabs and tooltip verification passed, including mutation rejection.'
    exit 0
} catch { Write-Error $_; exit 1 }

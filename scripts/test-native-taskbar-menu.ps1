$ErrorActionPreference='Stop'
# Source-level checks that the taskbar context menu stays visible when its owner is the
# taskbar window embedded in the shell. Inspects source only; it does not create windows.
$RepoRoot=Split-Path -Parent $PSScriptRoot
function Read-Source($relative){ [IO.File]::ReadAllText((Join-Path $RepoRoot $relative)) }
function Test-TaskbarMenu($menu){
    $failures=@()
    if($menu -notmatch 'bool ForeignHosted\(HWND owner\)'){ $failures+='Foreign-hosted owner detection missing' }
    if($menu -notmatch 'popup\.host=hosted\?InProcessHost\(owner\):owner'){ $failures+='Hosted owner is not replaced by an in-process host' }
    $create=[regex]::Match($menu,'CreateWindowExW\(WS_EX_TOOLWINDOW[^;]*?\);')
    if(-not $create.Success -or $create.Value -notmatch ',host,nullptr'){ $failures+='Menu popup is not created under the in-process host' }
    if($menu -notmatch 'if\(hosted && GetForegroundWindow\(\)!=popup\.window\)'){ $failures+='No system-menu fallback when the hosted popup cannot take the foreground' }
    if($menu -notmatch 'PostMessageW\(front,WM_NULL,0,0\)'){ $failures+='System-menu fallback omits the foreground and null-message pairing' }
    return ,$failures
}
$menu=Read-Source 'TrafficMonitor/NativeMenus/SearchableMenu.cpp'
$real=Test-TaskbarMenu $menu
if($real.Count){ $real | ForEach-Object { Write-Host "FAIL $_" }; exit 1 }
Write-Host 'PASS taskbar menu uses an in-process host with a system-menu fallback'
$mutant=$menu.Replace(',host,nullptr',',owner,nullptr')
if($mutant -ceq $menu){ throw 'Owner mutation did not apply' }
if((Test-TaskbarMenu $mutant).Count -eq 0){ Write-Host 'FAIL foreign owner mutation was not rejected'; exit 1 }
$mutant=$menu.Replace('if(hosted && GetForegroundWindow()!=popup.window)','if(false)')
if($mutant -ceq $menu){ throw 'Fallback mutation did not apply' }
if((Test-TaskbarMenu $mutant).Count -eq 0){ Write-Host 'FAIL fallback removal was not rejected'; exit 1 }
Write-Host 'PASS mutations rejected'

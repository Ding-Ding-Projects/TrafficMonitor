$ErrorActionPreference='Stop'
# Source-level checks that no owned native window keeps a system caption or system border.
# This inspects the creation styles and title bar wiring; it does not create windows.
$RepoRoot=Split-Path -Parent $PSScriptRoot
function Read-Source($relative){ [IO.File]::ReadAllText((Join-Path $RepoRoot $relative)) }
function Test-FramelessSources($workspace,$banner,$menu){
    $failures=@()
    if($workspace -notmatch 'm_titleBar\.Attach\(this,'){ $failures+='Workspace does not attach the Material title bar' }
    if($workspace -notmatch 'm_titleBar\.OuterSizeForContent\('){ $failures+='Workspace does not size itself through OuterSizeForContent' }
    if($workspace -notmatch 'm_titleBar\.SetAppearance\('){ $failures+='Workspace does not re-apply appearance to the title bar' }
    if($workspace -match 'AdjustWindowRectEx'){ $failures+='Workspace still derives sizes from the system caption through AdjustWindowRectEx' }
    $bannerCreate=[regex]::Match($banner,'CreateEx\([^;]*;')
    if(-not $bannerCreate.Success){ $failures+='Banner CreateEx call not found' }
    elseif($bannerCreate.Value -match 'WS_CAPTION|WS_BORDER|WS_DLGFRAME|WS_THICKFRAME'){ $failures+='Banner window style still has a system caption or border' }
    if($banner -match 'AdjustWindowRectEx'){ $failures+='Banner still sizes itself around a system frame' }
    $menuCreate=[regex]::Match($menu,'CreateWindowExW\(WS_EX_TOOLWINDOW[^;]*?\);')
    if(-not $menuCreate.Success){ $failures+='Menu popup CreateWindowExW call not found' }
    elseif($menuCreate.Value -match 'WS_CAPTION|WS_BORDER|WS_DLGFRAME|WS_THICKFRAME'){ $failures+='Menu popup style still has a system border' }
    return ,$failures
}
$workspace=Read-Source 'TrafficMonitor/NativeDashboard/Workspace.cpp'
$banner=Read-Source 'TrafficMonitor/NativeUpdate/BannerWindow.cpp'
$menu=Read-Source 'TrafficMonitor/NativeMenus/SearchableMenu.cpp'
$real=Test-FramelessSources $workspace $banner $menu
if($real.Count -gt 0){ $real | ForEach-Object { Write-Host "FAIL $_" }; exit 1 }
Write-Host 'PASS workspace title bar attached, banner and menu popup have no system caption or border'
# Mutation: put the system border back on a disposable in-memory copy of the menu popup.
$mutantMenu=$menu.Replace('WS_POPUP,anchor.x','WS_POPUP|WS_BORDER,anchor.x')
if($mutantMenu -ceq $menu){ throw 'Menu mutation did not apply' }
$mutant=Test-FramelessSources $workspace $banner $mutantMenu
if($mutant.Count -eq 0 -or ($mutant -join "`n") -notmatch 'Menu popup style'){ Write-Host 'FAIL border mutation was not rejected'; exit 1 }
Write-Host 'Expected rejection: disposable WS_BORDER mutation was rejected.'
$mutantBanner=$banner.Replace('WS_POPUP | WS_CLIPCHILDREN','WS_POPUP | WS_CAPTION | WS_CLIPCHILDREN')
if($mutantBanner -ceq $banner){ throw 'Banner mutation did not apply' }
if((Test-FramelessSources $workspace $mutantBanner $menu).Count -eq 0){ Write-Host 'FAIL caption mutation was not rejected'; exit 1 }
Write-Host 'Expected rejection: disposable WS_CAPTION mutation was rejected.'
exit 0
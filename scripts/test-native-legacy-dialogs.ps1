# Verifies that the converted legacy dialogs opt into the Material shell and leave no stock control behind.
#
# The dialogs need the running application (theApp, localization tables, resources), so this is a source and
# resource aware check, not a runtime check. For every converted dialog class it asserts:
#   1. the constructor calls EnableMaterialShell(), and FitMaterialContent / FinishMaterialShell / LayoutMaterialControls exist;
#   2. every control of its dialog template in TrafficMonitor.rc is either of a kind that
#      CBaseDialog::ConvertRemainingStockControls converts (static text, push button, check box, group box)
#      or is named by an explicit replace / remove / destroy call in the dialog source;
#   3. the dialog source and header declare no stock control member (CComboBox, CEdit, CButton, CStatic, CLinkStatic,
#      CSpinEdit, CPictureStatic ...), no DDX_Control, and use no CheckDlgButton / IsDlgButtonChecked;
#   4. only the converted dialogs opt into the shell, and the base class wires the shell into OnInitDialog.
# A disposable-copy mutation pass proves each assertion can fail.
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [switch]$SkipMutation
)
$ErrorActionPreference = 'Stop'

# class name, source base name, template id, documented stock allowances (none unless the conversion plan says so)
$Dialogs = @(
    @{ Class = 'CAboutDlg';                 File = 'AboutDlg';                 Allow = @('CToolTipCtrl') },
    @{ Class = 'CAppAlreadyRuningDlg';      File = 'AppAlreadyRuningDlg';      Allow = @() },
    @{ Class = 'CMessageDlg';               File = 'MessageDlg';               Allow = @() },
    @{ Class = 'CIconSelectDlg';            File = 'IconSelectDlg';            Allow = @() },
    @{ Class = 'CSkinAutoAdaptSettingDlg';  File = 'SkinAutoAdaptSettingDlg';  Allow = @() },
    @{ Class = 'CAutoAdaptSettingsDlg';     File = 'CAutoAdaptSettingsDlg';    Allow = @() },
    @{ Class = 'CWin11TaskbarSettingDlg';   File = 'Win11TaskbarSettingDlg';   Allow = @() },
    @{ Class = 'COptionsDlg';               File = 'OptionsDlg';               Allow = @() }
)
$ForbiddenStock = 'CLinkStatic|CSpinEdit|CComboBox|CEdit|CButton|CStatic|CListCtrl|CListBox|CSliderCtrl|CSpinButtonCtrl|CPictureStatic|CTabCtrl|CMFCEditBrowseCtrl'

function Get-DialogTemplate([string]$rcText, [string]$id)
{
    $m = [regex]::Match($rcText, "(?ms)^$id\s+DIALOGEX[^\r\n]*\r?\n(.*?)^END", 'Multiline')
    if (-not $m.Success) { return $null }
    $body = $m.Groups[1].Value
    $begin = $body.IndexOf("BEGIN")
    if ($begin -lt 0) { return @() }
    $lines = $body.Substring($begin + 5) -split "\r?\n"
    $controls = @()
    $pending = ''
    foreach ($raw in $lines) {
        $line = $raw.Trim()
        if (-not $line) { continue }
        $pending = if ($pending) { "$pending $line" } else { $line }
        if ($pending.EndsWith(',')) { continue }
        if ($pending -match '^(?<kw>[A-Z0-9]+)\s+(?<rest>.*)$') {
            $kw = $Matches['kw']
            $rest = $Matches['rest']
            $id = if ($kw -in 'EDITTEXT', 'COMBOBOX', 'LISTBOX', 'SCROLLBAR') { ($rest -split ',')[0].Trim() }
                  elseif ($rest -match '^"[^"]*"\s*,\s*([A-Za-z0-9_\-]+)') { $Matches[1] } else { ($rest -split ',')[0].Trim() }
            $kind = switch ($kw) {
                { $_ -in 'LTEXT', 'CTEXT', 'RTEXT' } { 'generic' }
                { $_ -in 'PUSHBUTTON', 'DEFPUSHBUTTON' } { 'generic' }
                'GROUPBOX' { 'generic' }
                'CONTROL' {
                    if ($rest -match '"Button"' -and $rest -match 'BS_(AUTO)?CHECKBOX') { 'generic' } else { 'special' }
                }
                default { 'special' }
            }
            $controls += [pscustomobject]@{ Keyword = $kw; Id = $id; Kind = $kind; Line = $pending }
        }
        $pending = ''
    }
    return $controls
}

# ---- Options host, tab page base and tab pages (lane: Options, TabDlg, General) ----------------------------------------
# A tab page is a child dialog of the Options dialog. It opts into the Material page mode instead of the shell, is laid out
# by LayoutPage, and its host is the Material tab bar. Other lanes add their pages to this list.
$TabPageDialogs = @(
    @{ Class = 'CGeneralSettingsDlg'; File = 'GeneralSettingsDlg' }
)

function Remove-CppComments([string]$text)
{
    $t = [regex]::Replace($text, '(?s)/\*.*?\*/', '')
    return [regex]::Replace($t, '(?m)//.*$', '')
}

function Test-OptionsHostAndTabBase([string]$src, $failures)
{
    # The scrolling page base: a Material scroll rail, no system scroll bar.
    $tabCpp = Remove-CppComments ([IO.File]::ReadAllText((Join-Path $src 'TabDlg.cpp')))
    $tabH = Remove-CppComments ([IO.File]::ReadAllText((Join-Path $src 'TabDlg.h')))
    foreach ($m in [regex]::Matches("$tabCpp`n$tabH", '\b(SB_VERT|SetScrollInfo|GetScrollInfo|ON_WM_VSCROLL|OnVScroll|WS_VSCROLL|SB_THUMBTRACK)\b')) {
        $failures.Add("TabDlg: still uses the system scroll bar ($($m.Groups[1].Value))")
    }
    if ($tabH -notmatch 'NativeMaterial::ScrollRail') { $failures.Add('TabDlg.h: the page base does not own a NativeMaterial::ScrollRail') }
    if ($tabCpp -notmatch 'm_rail\.SetMetrics') { $failures.Add('TabDlg.cpp: the scroll rail never receives the content metrics') }
    if ($tabCpp -notmatch 'm_rail\.Create') { $failures.Add('TabDlg.cpp: the scroll rail is never created') }
    if ($tabH -notmatch 'virtual int LayoutPage') { $failures.Add('TabDlg.h: LayoutPage is not declared') }
    if ($tabCpp -notmatch 'CTabDlg::FinishMaterialPage') { $failures.Add('TabDlg.cpp: FinishMaterialPage is not implemented') }

    # The host: the Material tab bar replaces the system tab control.
    $optCpp = Remove-CppComments ([IO.File]::ReadAllText((Join-Path $src 'OptionsDlg.cpp')))
    $optH = Remove-CppComments ([IO.File]::ReadAllText((Join-Path $src 'OptionsDlg.h')))
    foreach ($m in [regex]::Matches("$optCpp`n$optH", '\b(CTabCtrlEx|CTabCtrl|SysTabControl32)\b')) {
        $failures.Add("OptionsDlg: still uses the system tab control ($($m.Groups[1].Value))")
    }
    if ($optH -notmatch 'NativeMaterial::Tabs\s+m_tab') { $failures.Add('OptionsDlg.h: the tab bar is not a NativeMaterial::Tabs') }
    if ($optCpp -notmatch 'm_tab\.BindPage') { $failures.Add('OptionsDlg.cpp: the pages are not bound to the Material tab bar') }
    if ($optCpp -notmatch 'm_tab\.AddTab') { $failures.Add('OptionsDlg.cpp: no tab is added to the Material tab bar') }
    if ($optCpp -notmatch 'Create\(IDD_GENERAL_SETTINGS_DIALOG,\s*this\)') { $failures.Add('OptionsDlg.cpp: the pages are not children of the Options dialog') }
    if ($optCpp -notmatch 'HandleTabNavigation') { $failures.Add('OptionsDlg.cpp: Ctrl+Tab navigation of the Material tab bar is not wired') }
}

function Test-TabPageDialogs([string]$Root, $failures, [string]$rcText)
{
    $src = Join-Path $Root 'TrafficMonitor'
    Test-OptionsHostAndTabBase $src $failures
    foreach ($d in $TabPageDialogs) {
        $name = $d.File
        $cpp = [IO.File]::ReadAllText((Join-Path $src "$name.cpp"))
        $h = [IO.File]::ReadAllText((Join-Path $src "$name.h"))
        $ctor = "(?ms)$($d.Class)::$($d.Class)\s*\(.*?^\s*\{(?<body>.*?)^\}"
        $ctorBody = [regex]::Match($cpp, $ctor).Groups['body'].Value
        if ($ctorBody -notmatch '\bEnableMaterialPage\(\)') { $failures.Add("$name.cpp: constructor does not call EnableMaterialPage()") }
        if ($cpp -notmatch 'FinishMaterialPage\(\)') { $failures.Add("$name.cpp: FinishMaterialPage() is never called") }
        if ($cpp -notmatch "$($d.Class)::LayoutPage") { $failures.Add("$name.cpp: LayoutPage is not implemented") }
        if ($h -notmatch 'LayoutPage') { $failures.Add("$name.h: LayoutPage is not declared") }

        $stripped = Remove-CppComments "$cpp`n$h"
        foreach ($m in [regex]::Matches($stripped, "\b($ForbiddenStock|CToolTipCtrl|CComboBox2|CProgressCtrl)\b")) {
            $failures.Add("${name}: declares or uses the stock control type $($m.Groups[1].Value)")
        }
        if ($stripped -match '\bDDX_Control\b') { $failures.Add("${name}: still subclasses a control with DDX_Control") }
        if ($stripped -match '\b(CheckDlgButton|IsDlgButtonChecked|CheckRadioButton|GetCheckedRadioButton)\b') { $failures.Add("${name}: uses the stock check box helpers instead of SetDlgCheck / GetDlgCheck") }
        if ($stripped -match '\b(AfxMessageBox|MessageBox|SHMessageBoxCheck)\s*\(') { $failures.Add("${name}: opens a system message box") }
        if ($stripped -match '\bRelayEvent\b|\bWM_SPIN_EDIT_POS_CHANGED\b') { $failures.Add("${name}: still relays events to a stock tool tip or spin edit") }
        if ($stripped -match '\b(SB_VERT|SetScrollInfo|GetScrollInfo)\b') { $failures.Add("${name}: uses the system scroll bar") }
        if ($stripped -notmatch 'NativeMaterial::Tooltip\s+m_tooltip') { $failures.Add("${name}: tool tips do not use NativeMaterial::Tooltip") }

        $idd = [regex]::Match($cpp, "$($d.Class)::$($d.Class)\s*\([^)]*\)\s*:\s*CTabDlg\(\s*(IDD_\w+)").Groups[1].Value
        if (-not $idd) { $failures.Add("$name.cpp: cannot find the dialog template id"); continue }
        $controls = Get-DialogTemplate $rcText $idd
        if ($null -eq $controls) { $failures.Add("$name.cpp: template $idd not found in TrafficMonitor.rc"); continue }
        $lines = $cpp -split "\r?\n"
        foreach ($control in $controls) {
            if ($control.Kind -eq 'generic') { continue }
            $id = $control.Id
            $handled = $false
            for ($i = 0; $i -lt $lines.Count -and -not $handled; $i++) {
                if ($lines[$i] -notmatch "\b$([regex]::Escape($id))\b") { continue }
                $window = ($lines[[Math]::Max(0, $i - 1)..[Math]::Min($lines.Count - 1, $i + 4)]) -join ' '
                if ($window -match 'ReplaceWith\w+|ReplaceGroupBoxWithHeading|RemoveStockControl|DestroyWindow') { $handled = $true }
            }
            if (-not $handled) { $failures.Add("$name.cpp: template control $id ($($control.Keyword)) is not replaced or removed") }
        }
        Write-Host ("  {0}: {1} template controls ({2} converted generically, {3} explicitly)" -f $d.Class, $controls.Count, @($controls | Where-Object Kind -eq 'generic').Count, @($controls | Where-Object Kind -eq 'special').Count)
    }
    # Only the listed pages opt into the page mode.
    foreach ($file in Get-ChildItem $src -Filter *.cpp) {
        if ($file.Name -in 'BaseDialog.cpp', 'BaseDialogMaterial.cpp') { continue }
        $text = [IO.File]::ReadAllText($file.FullName)
        if ($text -match 'EnableMaterialPage\(') {
            $owner = $TabPageDialogs | Where-Object { $_.File + '.cpp' -eq $file.Name }
            if (-not $owner) { $failures.Add("$($file.Name): opts into the Material page mode but is not in the tab page list of this check") }
        }
    }
}

function Test-LegacyDialogs([string]$Root)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $src = Join-Path $Root 'TrafficMonitor'
    $rcText = [IO.File]::ReadAllText((Join-Path $src 'TrafficMonitor.rc'), [Text.Encoding]::Unicode)
    $basePath = Join-Path $src 'BaseDialog.cpp'
    $materialPath = Join-Path $src 'BaseDialogMaterial.cpp'
    $baseCpp = [IO.File]::ReadAllText($basePath)
    $material = [IO.File]::ReadAllText($materialPath)

    # 4. The base wires the shell and converts the generic kinds.
    if ($baseCpp -notmatch 'if\s*\(\s*m_material_shell\s*\)\s*AttachMaterialShell\(\)') { $failures.Add('BaseDialog.cpp: OnInitDialog does not attach the Material shell for an opted-in dialog') }
    $convert = [regex]::Match($material, '(?s)void CBaseDialog::ConvertRemainingStockControls\(\).*?\r?\n}\r?\n').Value
    foreach ($call in 'ReplaceWithLabel', 'ReplaceWithButton', 'ReplaceWithCheckbox', 'ReplaceGroupBoxWithHeading') {
        if ($convert -notmatch $call) { $failures.Add("BaseDialogMaterial.cpp: ConvertRemainingStockControls no longer calls $call") }
    }
    if ($material -notmatch 'TitleBar::Attach|m_title_bar\.Attach') { $failures.Add('BaseDialogMaterial.cpp: the shell does not attach the title bar') }

    $converted = @{}
    foreach ($d in $Dialogs) {
        $name = $d.File
        $cppPath = Join-Path $src "$name.cpp"
        $hPath = Join-Path $src "$name.h"
        $cpp = [IO.File]::ReadAllText($cppPath)
        $h = [IO.File]::ReadAllText($hPath)
        $converted[$d.Class] = $true

        # 1. Opt-in and lifecycle.
        # The constructor body starts at the first line that is only an opening brace and ends at the next closing brace in column 0.
        $ctor = "(?ms)$($d.Class)::$($d.Class)\s*\(.*?^\s*\{(?<body>.*?)^\}"
        $ctorBody = [regex]::Match($cpp, $ctor).Groups['body'].Value
        if ($ctorBody -notmatch '\bEnableMaterialShell\(\)') { $failures.Add("$name.cpp: constructor does not call EnableMaterialShell()") }
        if ($cpp -notmatch 'FinishMaterialShell\(\)') { $failures.Add("$name.cpp: FinishMaterialShell() is never called") }
        if ($cpp -notmatch 'FitMaterialContent\(') { $failures.Add("$name.cpp: FitMaterialContent() is never called") }
        if ($cpp -notmatch "$($d.Class)::LayoutMaterialControls") { $failures.Add("$name.cpp: LayoutMaterialControls is not implemented") }
        if ($h -notmatch 'LayoutMaterialControls') { $failures.Add("$name.h: LayoutMaterialControls is not declared") }

        # 3. No stock control members or stock check box helpers.
        $combined = "$cpp`n$h"
        $stripped = [regex]::Replace($combined, '(?s)/\*.*?\*/', '')
        $stripped = [regex]::Replace($stripped, '(?m)//.*$', '')
        foreach ($m in [regex]::Matches($stripped, "\b($ForbiddenStock)\b")) {
            $failures.Add("${name}: declares or uses the stock control type $($m.Groups[1].Value)")
        }
        foreach ($m in [regex]::Matches($stripped, '\b(CToolTipCtrl)\b')) {
            if ($d.Allow -notcontains $m.Groups[1].Value) { $failures.Add("${name}: uses the stock tool tip control without a documented allowance") }
        }
        if ($stripped -match '\bDDX_Control\b') { $failures.Add("${name}: still subclasses a control with DDX_Control") }
        if ($stripped -match '\b(CheckDlgButton|IsDlgButtonChecked)\b') { $failures.Add("${name}: uses the stock check box helpers instead of SetDlgCheck / GetDlgCheck") }
        if ($stripped -match '\b(AfxMessageBox|MessageBox)\s*\(') { $failures.Add("${name}: opens a system message box") }

        # 2. Every template control is converted generically or named by an explicit call.
        $idd = [regex]::Match($cpp, "$($d.Class)::$($d.Class)\s*\([^)]*\)\s*:\s*CBaseDialog\(\s*(IDD_\w+)").Groups[1].Value
        if (-not $idd) { $failures.Add("$name.cpp: cannot find the dialog template id"); continue }
        $controls = Get-DialogTemplate $rcText $idd
        if ($null -eq $controls) { $failures.Add("$name.cpp: template $idd not found in TrafficMonitor.rc"); continue }
        $lines = $cpp -split "\r?\n"
        foreach ($control in $controls) {
            if ($control.Kind -eq 'generic') { continue }
            $id = $control.Id
            $handled = $false
            for ($i = 0; $i -lt $lines.Count -and -not $handled; $i++) {
                if ($lines[$i] -notmatch "\b$([regex]::Escape($id))\b") { continue }
                $window = ($lines[[Math]::Max(0, $i - 1)..[Math]::Min($lines.Count - 1, $i + 4)]) -join ' '
                if ($window -match 'ReplaceWith\w+|ReplaceGroupBoxWithHeading|RemoveStockControl|fold\(|DestroyWindow') { $handled = $true }
            }
            if (-not $handled) { $failures.Add("$name.cpp: template control $id ($($control.Keyword)) is not replaced or removed") }
        }
        Write-Host ("  {0}: {1} template controls ({2} converted generically, {3} explicitly)" -f $d.Class, $controls.Count, @($controls | Where-Object Kind -eq 'generic').Count, @($controls | Where-Object Kind -eq 'special').Count)
    }

    # 4. Only the converted dialogs opt in.
    foreach ($file in Get-ChildItem $src -Filter *.cpp) {
        $text = [IO.File]::ReadAllText($file.FullName)
        if ($file.Name -in 'BaseDialog.cpp', 'BaseDialogMaterial.cpp') { continue }
        if ($text -match 'EnableMaterialShell\(') {
            $owner = $Dialogs | Where-Object { $_.File + '.cpp' -eq $file.Name }
            if (-not $owner) { $failures.Add("$($file.Name): opts into the Material shell but is not in the converted dialog list of this check") }
        }
    }
    Test-TabPageDialogs $Root $failures $rcText
    return , $failures
}

Write-Host 'Legacy dialog conversion check (source and resource scan; the dialogs cannot run without the application)'
$failures = Test-LegacyDialogs $Root
if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Host "FAIL $_" }
    Write-Host "FAIL $($failures.Count) legacy dialog conversion assertions failed"
    exit 1
}
Write-Host 'PASS every converted dialog opts into the Material shell and leaves no stock control behind'

if ($SkipMutation) { exit 0 }

# Mutation pass: each mutation is applied to a disposable copy and must be rejected with the expected message.
$work = Join-Path $Root ('artifacts/legacy-dialogs-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $work 'TrafficMonitor') | Out-Null
try {
    $mutations = @(
        @{ Name = 'constructor opt-in removed'; File = 'AboutDlg.cpp'; Find = 'EnableMaterialShell();'; Replace = ''; Expect = 'AboutDlg.cpp: constructor does not call EnableMaterialShell' },
        @{ Name = 'combo replacement removed'; File = 'IconSelectDlg.cpp'; Find = 'ReplaceWithDropdown(IDC_COMBO1, select_caption)'; Replace = 'nullptr'; Expect = 'IconSelectDlg.cpp: template control IDC_COMBO1' },
        @{ Name = 'stock member reintroduced'; File = 'SkinAutoAdaptSettingDlg.h'; Find = 'NativeMaterial::Button* m_ok{};'; Replace = 'NativeMaterial::Button* m_ok{}; CComboBox m_stock_combo;'; Expect = 'SkinAutoAdaptSettingDlg: declares or uses the stock control type CComboBox' },
        @{ Name = 'shell never finished'; File = 'MessageDlg.cpp'; Find = 'FinishMaterialShell();'; Replace = ''; Expect = 'MessageDlg.cpp: FinishMaterialShell() is never called' },
        @{ Name = 'edit replacement removed'; File = 'Win11TaskbarSettingDlg.cpp'; Find = 'fold(IDC_WIDTET_WIDTH_EDIT, IDC_WIDGET_WIDTH_STATIC, IDC_PIXEL_STATIC2)'; Replace = 'nullptr'; Expect = 'Win11TaskbarSettingDlg.cpp: template control IDC_WIDTET_WIDTH_EDIT' },
        @{ Name = 'stock check helper used'; File = 'CAutoAdaptSettingsDlg.cpp'; Find = 'GetDlgCheck(IDC_AUTO_SAVE_TO_PRESET_CHECK)'; Replace = '(IsDlgButtonChecked(IDC_AUTO_SAVE_TO_PRESET_CHECK) != 0)'; Expect = 'CAutoAdaptSettingsDlg: uses the stock check box helpers' },
        @{ Name = 'base stops attaching the shell'; File = 'BaseDialog.cpp'; Find = 'AttachMaterialShell();      //'; Replace = ';      //'; Expect = 'BaseDialog.cpp: OnInitDialog does not attach the Material shell' },
        # lane: Options, TabDlg, General
        @{ Name = 'options tab control returns'; File = 'OptionsDlg.h'; Find = 'NativeMaterial::Tabs m_tab;'; Replace = 'CTabCtrl m_tab;'; Expect = 'OptionsDlg: still uses the system tab control' },
        @{ Name = 'options pages not bound to the tab bar'; File = 'OptionsDlg.cpp'; Find = 'm_tab.BindPage('; Replace = 'm_tab.GetTabText('; Expect = 'OptionsDlg.cpp: the pages are not bound to the Material tab bar' },
        @{ Name = 'options tab control not removed'; File = 'OptionsDlg.cpp'; Find = 'RemoveStockControl(IDC_TAB1);'; Replace = ''; Expect = 'OptionsDlg.cpp: template control IDC_TAB1' },
        @{ Name = 'page base scrolls with the system bar'; File = 'TabDlg.cpp'; Find = 'm_rail.SetMetrics(m_content_height, m_viewport_height, m_scroll_pos);'; Replace = 'm_rail.SetMetrics(m_content_height, m_viewport_height, m_scroll_pos); SetScrollInfo(SB_VERT, nullptr, TRUE);'; Expect = 'TabDlg: still uses the system scroll bar' },
        @{ Name = 'page base stops creating the rail'; File = 'TabDlg.cpp'; Find = 'if (!m_rail.Create(this, RailControlId))'; Replace = 'if (false)'; Expect = 'TabDlg.cpp: the scroll rail is never created' },
        @{ Name = 'general page opt-in removed'; File = 'GeneralSettingsDlg.cpp'; Find = 'EnableMaterialPage();'; Replace = ''; Expect = 'GeneralSettingsDlg.cpp: constructor does not call EnableMaterialPage' },
        @{ Name = 'general page never finished'; File = 'GeneralSettingsDlg.cpp'; Find = 'FinishMaterialPage();'; Replace = ''; Expect = 'GeneralSettingsDlg.cpp: FinishMaterialPage() is never called' },
        @{ Name = 'general edit replacement removed'; File = 'GeneralSettingsDlg.cpp'; Find = 'ReplaceWithNumberStepper(IDC_MBD_TEMP_TIP_EDIT, threshold_label, unit_of(IDC_MBD_TEMP_STATIC))'; Replace = 'nullptr'; Expect = 'GeneralSettingsDlg.cpp: template control IDC_MBD_TEMP_TIP_EDIT' },
        @{ Name = 'general radio replacement removed'; File = 'GeneralSettingsDlg.cpp'; Find = 'ReplaceWithRadioButton(IDC_USE_PDH_RADIO, false)'; Replace = 'nullptr'; Expect = 'GeneralSettingsDlg.cpp: template control IDC_USE_PDH_RADIO' },
        @{ Name = 'general combo replacement removed'; File = 'GeneralSettingsDlg.cpp'; Find = 'ReplaceWithDropdown(IDC_SELECT_CPU_COMBO, cpu_caption)'; Replace = 'nullptr'; Expect = 'GeneralSettingsDlg.cpp: template control IDC_SELECT_CPU_COMBO' },
        @{ Name = 'general stock tool tip returns'; File = 'GeneralSettingsDlg.h'; Find = 'NativeMaterial::Tooltip m_tooltip;'; Replace = 'NativeMaterial::Tooltip m_tooltip; CToolTipCtrl m_stock_tip;'; Expect = 'GeneralSettingsDlg: declares or uses the stock control type CToolTipCtrl' },
        @{ Name = 'general stock spin edit returns'; File = 'GeneralSettingsDlg.h'; Find = 'NativeMaterial::Tooltip m_tooltip;'; Replace = 'NativeMaterial::Tooltip m_tooltip; CSpinEdit m_stock_spin;'; Expect = 'GeneralSettingsDlg: declares or uses the stock control type CSpinEdit' },
        @{ Name = 'general check helper used'; File = 'GeneralSettingsDlg.cpp'; Find = 'GetDlgCheck(IDC_CPU_TEMP_TIP_CHECK)'; Replace = '(IsDlgButtonChecked(IDC_CPU_TEMP_TIP_CHECK) != 0)'; Expect = 'GeneralSettingsDlg: uses the stock check box helpers' },
        @{ Name = 'general system message box returns'; File = 'GeneralSettingsDlg.cpp'; Find = 'return NativeMaterial::Alert(m_hWnd, CCommon::LoadText(IDS_HARDWARE_MONITOR_WARNING), APP_NAME, MB_OKCANCEL | MB_ICONWARNING) == IDOK;'; Replace = 'return SHMessageBoxCheck(m_hWnd, CCommon::LoadText(IDS_HARDWARE_MONITOR_WARNING), APP_NAME, MB_OKCANCEL, IDOK, L"x") == IDOK;'; Expect = 'GeneralSettingsDlg: opens a system message box' }
    )
    $srcFiles = Get-ChildItem (Join-Path $Root 'TrafficMonitor') -File | Where-Object { $_.Extension -in '.cpp', '.h', '.rc' }
    foreach ($mutation in $mutations) {
        $dest = Join-Path $work 'TrafficMonitor'
        Get-ChildItem $dest -File | Remove-Item -Force
        foreach ($file in $srcFiles) { Copy-Item $file.FullName (Join-Path $dest $file.Name) }
        $target = Join-Path $dest $mutation.File
        $text = [IO.File]::ReadAllText($target)
        if (-not $text.Contains($mutation.Find)) { Write-Host "FAIL mutation '$($mutation.Name)': text to mutate not found in $($mutation.File)"; exit 1 }
        $mutated = $text.Replace($mutation.Find, $mutation.Replace)
        [IO.File]::WriteAllText($target, $mutated)
        $result = Test-LegacyDialogs $work 6>$null
        $messages = @($result | ForEach-Object { $_ }) | Where-Object { $_ -is [string] -and $_ -match '^[A-Za-z0-9_\.]+' }
        $hit = $messages | Where-Object { $_ -like "*$($mutation.Expect)*" }
        if (-not $hit) {
            Write-Host "FAIL mutation '$($mutation.Name)' was not rejected (expected: $($mutation.Expect); got: $($messages -join ' | '))"
            exit 1
        }
        Write-Host "Expected rejection: mutation '$($mutation.Name)' -> $($hit | Select-Object -First 1)"
    }
    Write-Host "PASS $($mutations.Count) mutations were rejected"
}
finally {
    if (Test-Path $work) { Remove-Item $work -Recurse -Force }
}
exit 0

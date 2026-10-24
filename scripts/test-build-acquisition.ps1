$ErrorActionPreference = 'Stop'
$source = Get-Content -Raw (Join-Path $PSScriptRoot 'build-tools.ps1')
$parseErrors = $null
$tokens = $null
$ast = [System.Management.Automation.Language.Parser]::ParseInput($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Build tools script did not parse' }
$finder = $ast.Find({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Find-BuildInstallation' }, $true)
if (!$finder) { throw 'Missing discovery function' }
# Load only discovery, so the regression cannot acquire or launch an installer.
Invoke-Expression $finder.Extent.Text
function Assert($Condition, $Message) { if (!$Condition) { throw $Message } }
function Mock-VsWhere {
    $script:DiscoveryArguments = @($args)
    $recovery = $args -contains '-all'
    # Model an incomplete 2022 instance missing VC packages and a newer 2026 instance.
    if ($recovery -and $args -notcontains '-requires' -and
        $args -contains 'Microsoft.VisualStudio.Product.BuildTools' -and
        $args -contains '[17.0,18.0)') { return 'C:\Existing2022' }
    if ($script:Ready -and !$recovery -and $args -contains 'Microsoft.VisualStudio.Component.Windows11SDK.22621') { return 'C:\Ready2022' }
}
$script:Ready = $false
Assert (!(Find-BuildInstallation Mock-VsWhere)) 'Incomplete instance was accepted as ready'
Assert ($script:DiscoveryArguments -notcontains '-all') 'Ready discovery admitted incomplete instances'
foreach ($component in @('Microsoft.VisualStudio.Component.VC.Tools.x86.x64', 'Microsoft.VisualStudio.Component.VC.ATLMFC', 'Microsoft.VisualStudio.Component.Windows11SDK.22621')) {
    Assert ($script:DiscoveryArguments -contains $component) "Ready discovery omitted $component"
}
Assert ((Find-BuildInstallation Mock-VsWhere -Recovery) -eq 'C:\Existing2022') 'Recovery missed incomplete 2022 instance'
Assert ($script:DiscoveryArguments -notcontains '-requires') 'Recovery requires possibly missing packages'
$script:Ready = $true
Assert ((Find-BuildInstallation Mock-VsWhere) -eq 'C:\Ready2022') 'Ready installation was not reused'
$argumentFunction = $ast.Find({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Get-BuildAcquisitionArguments' }, $true)
Assert ($null -ne $argumentFunction) 'Missing acquisition argument function'
Invoke-Expression $argumentFunction.Extent.Text
$ownedRoot = Join-Path $env:LOCALAPPDATA 'TrafficMonitorBuildTools'
$modify = Get-BuildAcquisitionArguments $PSScriptRoot $ownedRoot
Assert ($modify.StartsWith('modify --installPath ')) 'Extant incomplete instance was not modified'
Assert ($modify.Contains(('"{0}"' -f $PSScriptRoot))) 'Modify changed the existing destination'
foreach ($required in @('--add Microsoft.VisualStudio.Workload.VCTools', '--add Microsoft.VisualStudio.Component.VC.ATLMFC', '--add Microsoft.VisualStudio.Component.Windows11SDK.22621', '--includeRecommended', '--norestart')) {
    Assert ($modify.Contains($required)) "Modify omitted $required"
}
$missing = Join-Path $env:TEMP ([guid]::NewGuid().ToString())
Assert (!(Test-Path -LiteralPath $missing)) 'Missing-directory fixture unexpectedly exists'
foreach ($existing in @($missing, $null)) {
    $fresh = Get-BuildAcquisitionArguments $existing $ownedRoot
    Assert ($fresh.StartsWith(('--installPath "{0}" ' -f (Join-Path $ownedRoot 'VS2022')))) 'Fresh install did not use owned destination'
    Assert (!$fresh.Contains($missing)) 'Stale registration destination was reused'
}
foreach ($unsafe in @('C:\bad"path', 'relative\path', "C:\bad`npath")) {
    $rejected = $false
    try { Get-BuildAcquisitionArguments $unsafe $ownedRoot | Out-Null } catch { $rejected = $true }
    Assert $rejected 'Unsafe recovery path was accepted'
    $rejected = $false
    try { Get-BuildAcquisitionArguments $null $unsafe | Out-Null } catch { $rejected = $true }
    Assert $rejected 'Unsafe acquisition root was accepted'
}
# Both initial and post-install readiness must use the same strict discovery route.
$calls = $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.CommandAst] -and $node.GetCommandName() -eq 'Find-BuildInstallation' }, $true)
Assert ($calls.Count -eq 3) 'Expected initial readiness, recovery and final readiness queries'
Assert (@($calls | Where-Object { $_.Extent.Text -match '-Recovery' }).Count -eq 1) 'Recovery discovery escaped its acquisition-only role'
Write-Host 'PASS: strict readiness, incomplete-instance recovery, stale-directory isolation, unsafe-path rejection and complete modify components'

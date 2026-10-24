$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Security/Microsoft.PowerShell.Security.psd1') -ErrorAction Stop
$RepoRoot = Split-Path $PSScriptRoot -Parent
$ToolsRoot = Join-Path $env:LOCALAPPDATA 'TrafficMonitorBuildTools'
New-Item -ItemType Directory -Force $ToolsRoot | Out-Null
function Get-TrustedFile($Url, $Destination, $Hash) {
    if (!(Test-Path $Destination)) { Invoke-WebRequest -UseBasicParsing $Url -OutFile $Destination }
    if ($Hash) {
        if ((Get-FileHash $Destination -Algorithm SHA256).Hash -ne $Hash) { throw "Download checksum mismatch: $Destination" }
    } else {
        $signature = Get-AuthenticodeSignature $Destination
        if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') { throw 'Microsoft installer signature validation failed' }
    }
}
function Find-BuildInstallation($VsWhere, [switch]$Recovery) {
    if ($Recovery) {
        # Incomplete instances and instances missing VC tools still need recovery.
        # Never use this result as evidence that the toolchain is ready.
        return & $VsWhere -all -latest -products Microsoft.VisualStudio.Product.BuildTools -version '[17.0,18.0)' -property installationPath
    }
    return & $VsWhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.ATLMFC Microsoft.VisualStudio.Component.Windows11SDK.22621 -property installationPath
}
function Get-BuildAcquisitionArguments($ExistingInstallation, $AcquisitionRoot) {
    foreach ($path in @($ExistingInstallation, $AcquisitionRoot)) {
        if ($path -and ($path -match '["\r\n]' -or ![IO.Path]::IsPathRooted($path))) { throw 'Invalid Build Tools installation directory' }
    }
    $components = '--quiet --wait --norestart --nocache --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.ATLMFC --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended'
    if ($ExistingInstallation -and (Test-Path -LiteralPath $ExistingInstallation -PathType Container)) {
        return 'modify --installPath "{0}" --channelId Microsoft.VisualStudio.17.Release {1}' -f $ExistingInstallation, $components
    }
    # Preserve stale registrations without recreating another application's path.
    if (!$AcquisitionRoot) { throw 'Missing Build Tools acquisition root' }
    $destination = Join-Path $AcquisitionRoot 'VS2022'
    return '--installPath "{0}" {1}' -f $destination, $components
}
function Get-BuildInstallation {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $existingInstallation = $null
    if (Test-Path $vswhere) {
        $installation = Find-BuildInstallation $vswhere
        if ($installation) { return $installation }
        # The Build Tools bootstrapper cannot install over an existing instance.
        # Add missing components to its matching stable product instead.
        $existingInstallation = Find-BuildInstallation $vswhere -Recovery
    }
    $arguments = Get-BuildAcquisitionArguments $existingInstallation $ToolsRoot
    $bootstrap = Join-Path $ToolsRoot 'vs_buildtools.exe'
    Get-TrustedFile 'https://aka.ms/vs/17/release/vs_buildtools.exe' $bootstrap $null
    if ($arguments.StartsWith('modify ')) { Write-Host 'Visual Studio acquisition mode: add components to existing stable Build Tools instance' }
    else { Write-Host 'Visual Studio acquisition mode: install at TrafficMonitor-owned destination; preserve stale registrations' }
    $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    if (!$admin -and ($Silent -or $env:CI -eq 'true')) { throw 'Missing MSVC/MFC requires administrator installation. Silent mode cannot request native consent.' }
    $launchStage = if ($admin) { 'installer-launch' } else { 'native-elevation-request' }
    Write-Host "Visual Studio acquisition stage: $launchStage"
    try {
        # Shell elevation must receive an existing explicit working directory.
        # Keep consent native; do not replace a failed elevation with another route.
        if ($admin) { $process = Start-Process -FilePath $bootstrap -ArgumentList $arguments -WorkingDirectory $ToolsRoot -Wait -PassThru -WindowStyle Hidden }
        else { $process = Start-Process -FilePath $bootstrap -ArgumentList $arguments -WorkingDirectory $ToolsRoot -Verb RunAs -Wait -PassThru }
    } catch {
        $nativeCode = 'unavailable'
        $cause = $_.Exception
        $exceptionType = $cause.GetType().FullName
        while ($null -ne $cause) {
            if ($cause -is [System.ComponentModel.Win32Exception]) { $nativeCode = $cause.NativeErrorCode; break }
            $cause = $cause.InnerException
        }
        throw "Visual Studio acquisition failed at $launchStage; exception=$exceptionType; nativeErrorCode=$nativeCode. Native consent or installer launch did not complete."
    }
    Write-Host "Visual Studio acquisition stage: installer-exit; exitCode=$($process.ExitCode)"
    if ($process.ExitCode -ne 0) { throw "Visual Studio installation incomplete, exit $($process.ExitCode). A required restart must be performed manually." }
    if (!(Test-Path $vswhere)) { throw 'Visual Studio discovery tool missing after installation' }
    $installation = Find-BuildInstallation $vswhere
    if (!$installation) { throw 'MSVC v143, MFC and Windows SDK 22621 installation verification failed' }
    return $installation
}

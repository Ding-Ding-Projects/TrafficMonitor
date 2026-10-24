param(
    [Parameter(Mandatory=$true)][string]$PackagePath,
    [Parameter(Mandatory=$true)][string]$ExpectedPackageSHA256,
    [Parameter(Mandatory=$true)][string]$ExpectedExecutableSHA256,
    [Parameter(Mandatory=$true)][string]$SourceCommit,
    [Parameter(Mandatory=$true)][string]$FixtureName
)
$ErrorActionPreference = 'Stop'
function Get-StableProfileBaseline {
    param([string]$Path,
        [scriptblock]$ReadBytes = { param($File) [IO.File]::ReadAllBytes($File) },
        [scriptblock]$ReadLiveHash = { param($File) (Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash })
    $bytes = [byte[]](& $ReadBytes $Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { $retainedHash = ([BitConverter]::ToString($algorithm.ComputeHash($bytes))).Replace('-','') } finally { $algorithm.Dispose() }
    $liveHash = & $ReadLiveHash $Path
    if ($retainedHash -ne $liveHash) { throw 'Profile changed during baseline acquisition' }
    return @{path=$Path; sha256=$retainedHash; bytes=[Convert]::ToBase64String($bytes)}
}
$stage = 'inputs'
try {
    $root = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
    if ($FixtureName -notmatch '\A[a-zA-Z0-9][a-zA-Z0-9_-]{0,79}\z' -or $FixtureName -match '^(CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])$') { throw 'Invalid fixture name' }
    if ($SourceCommit -notmatch '\A[0-9a-fA-F]{40}\z' -or $ExpectedPackageSHA256 -notmatch '\A[0-9a-fA-F]{64}\z' -or $ExpectedExecutableSHA256 -notmatch '\A[0-9a-fA-F]{64}\z') { throw 'Invalid identity' }
    $resolvedSource = & git -C $root rev-parse --verify --quiet ($SourceCommit + '^{commit}') 2>$null
    if ($LASTEXITCODE -ne 0 -or $resolvedSource -cne $SourceCommit) { throw 'Exact source commit is unavailable' }
    $destination = Join-Path $root ('artifacts\diagnostics\' + $FixtureName)
    foreach ($forbidden in @([IO.Path]::GetTempPath(), $env:APPDATA, $env:LOCALAPPDATA)) {
        if ($forbidden -and $destination.StartsWith([IO.Path]::GetFullPath($forbidden).TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe output root' }
    }
    # Reject reparse ancestors, including an existing destination, before writing.
    $ancestor = $destination
    while ($ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -Force -LiteralPath $ancestor).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Reparse path' }
        $ancestor = Split-Path $ancestor -Parent
    }
    if (Test-Path -LiteralPath $destination) { throw 'Destination already exists' }
    & git -C $root check-ignore -q -- ('artifacts/diagnostics/'+$FixtureName+'/receipt.json')
    if ($LASTEXITCODE -ne 0) { throw 'Receipt destination is not ignored' }
    $stage = 'package-identity'
    if ((Get-FileHash -LiteralPath $PackagePath -Algorithm SHA256).Hash -ne $ExpectedPackageSHA256) { throw 'Package identity mismatch' }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $PackagePath).Path)
    try {
        $stage = 'archive-validation'
        $entries = @(); $names = @{}; [long]$total = 0
        foreach ($entry in $zip.Entries) {
            $name = $entry.FullName.Replace('\','/')
            if ($name -match '(^/|:|[\x00-\x1f])' -or @($name.Split('/') | Where-Object { $_ -eq '..' -or $_ -eq '.' }).Count) { throw 'Unsafe archive name' }
            if (!$name.StartsWith('lib/net45/',[StringComparison]::OrdinalIgnoreCase)) { continue }
            $relative = $name.Substring(10)
            if (!$relative -or $relative.EndsWith('/')) { continue }
            if (@($relative.Split('/') | Where-Object { !$_ -or $_ -match '[. ]$' -or $_ -match '^(CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])($|\.)' }).Count) { throw 'Unsafe runtime name' }
            $mode = ($entry.ExternalAttributes -shr 16) -band 0xF000
            if ($mode -eq 0xA000 -or ($entry.ExternalAttributes -band 0x400)) { throw 'Archive link or reparse entry' }
            if ([IO.Path]::GetFileName($relative) -ieq 'Update.exe') { throw 'Unexpected updater in runtime' }
            if ([IO.Path]::GetFileName($relative) -ieq 'installed-mode.ini') { continue }
            if ($names.ContainsKey($relative)) { throw 'Duplicate runtime destination' }; $names[$relative] = $true
            $total += $entry.Length
            if ($total -gt 536870912 -or $entries.Count -ge 10000) { throw 'Runtime extraction bound exceeded' }
            $entries += @{Entry=$entry; Relative=$relative}
        }
        if (!$names.ContainsKey('TrafficMonitor.exe')) { throw 'Runtime executable absent' }
        # A new directory is owned by this preparation only. Never overwrite a prior fixture.
        New-Item -ItemType Directory -Path $destination | Out-Null
        $runtime = Join-Path $destination 'runtime'; New-Item -ItemType Directory -Path $runtime | Out-Null
        foreach ($record in $entries) {
            $target = [IO.Path]::GetFullPath((Join-Path $runtime $record.Relative))
            if (!$target.StartsWith($runtime+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Runtime path escaped output' }
            [IO.Directory]::CreateDirectory((Split-Path $target -Parent)) | Out-Null
            $inputStream = $record.Entry.Open()
            try {
                $outputStream = [IO.File]::Open($target,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
                try {
                    $buffer = New-Object byte[] 65536; [long]$written = 0
                    while (($read = $inputStream.Read($buffer,0,$buffer.Length)) -gt 0) {
                        $written += $read
                        if ($written -gt $record.Entry.Length) { throw 'Archive entry exceeded declared size' }
                        $outputStream.Write($buffer,0,$read)
                    }
                    if ($written -ne $record.Entry.Length) { throw 'Archive entry was truncated' }
                } finally { $outputStream.Dispose() }
            } finally { $inputStream.Dispose() }
        }
    } finally { $zip.Dispose() }
    $stage = 'runtime-identity'
    if ((Get-FileHash -LiteralPath (Join-Path $runtime 'TrafficMonitor.exe') -Algorithm SHA256).Hash -ne $ExpectedExecutableSHA256) { throw 'Executable identity mismatch' }
    if (Test-Path -LiteralPath (Join-Path $runtime 'installed-mode.ini')) { throw 'Managed marker remains' }
    if (Get-ChildItem -LiteralPath $runtime -Recurse -Force -Filter Update.exe) { throw 'Updater remains' }
    # This program rejects portable persistence beneath TEMP. The fixture is outside it.
    # Fresh exact CRLF preferences select English and Light without normal-profile writes.
    [IO.File]::WriteAllText((Join-Path $runtime 'global_cfg.ini'),"[config]`r`nportable_mode=true`r`n",[Text.Encoding]::ASCII)
    [IO.File]::WriteAllText((Join-Path $runtime 'config.ini'),"[general]`r`ncheck_update_when_start=false`r`n[config]`r`nshow_task_bar_wnd=false`r`n",[Text.Encoding]::ASCII)
    [IO.File]::WriteAllText((Join-Path $runtime 'native-interface.ini'),"[interface]`r`nschemaVersion=1`r`nlanguage=0`r`nappearance=1`r`n",[Text.Encoding]::ASCII)
    $stage = 'normal-profile-baseline'
    $profile = Join-Path ([Environment]::GetFolderPath('ApplicationData')) 'TrafficMonitor'
    $baseline = @()
    if (Test-Path -LiteralPath $profile) {
        $profileItems = @(); $pending = @((Get-Item -Force -LiteralPath $profile))
        while ($pending.Count) {
            $item = $pending[0]; $pending = @($pending | Select-Object -Skip 1)
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Profile baseline contains reparse path' }
            $profileItems += $item
            if ($item.PSIsContainer) { $pending += @(Get-ChildItem -LiteralPath $item.FullName -Force) }
        }
        foreach ($file in $profileItems | Where-Object { !$_.PSIsContainer }) {
            $baseline += Get-StableProfileBaseline $file.FullName
        }
    }
    $runKey = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Software\Microsoft\Windows\CurrentVersion\Run')
    try { $present = $runKey -and ($runKey.GetValueNames() -contains 'TrafficMonitor'); $startup = @{present=[bool]$present}; if ($present) { $startup.kind=$runKey.GetValueKind('TrafficMonitor').ToString(); $startup.value=$runKey.GetValue('TrafficMonitor',$null,[Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames) } } finally { if ($runKey) { $runKey.Dispose() } }
    if ((Get-FileHash -LiteralPath $PackagePath -Algorithm SHA256).Hash -ne $ExpectedPackageSHA256) { throw 'Original package changed' }
    $receipt = @{schemaVersion=1; prepared=$true; launched=$false; sourceCommit=$SourceCommit; packageSHA256=$ExpectedPackageSHA256; executableSHA256=$ExpectedExecutableSHA256; runtime=$runtime; normalProfileExists=(Test-Path -LiteralPath $profile); normalProfile=$baseline; startup=$startup; preparedAtUtc=[DateTime]::UtcNow.ToString('o'); limitations='Preparation only. SourceCommit existence is validated; association of this package with that source requires an independently verified build receipt. No launch, audio, GUI or subsequent profile-preservation proof.'}
    [IO.File]::WriteAllText((Join-Path $destination 'receipt.json'),($receipt | ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding $false))
    & git -C $root check-ignore -q -- ('artifacts/diagnostics/'+$FixtureName+'/receipt.json')
    if ($LASTEXITCODE -ne 0) { throw 'Receipt ignore proof lost' }
    @{prepared=$true; launched=$false; stage='ready'; fixture=$FixtureName} | ConvertTo-Json -Compress
    exit 0
} catch {
    # Profile bytes, registry values, paths and fingerprints never reach stdout.
    @{prepared=$false; launched=$false; stage=$stage} | ConvertTo-Json -Compress
    exit 1
}

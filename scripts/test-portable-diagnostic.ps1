$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$root = Split-Path $PSScriptRoot -Parent
$source = (& git -C $root rev-parse HEAD).Trim()
$prefix = 'portable-preflight-test-' + [guid]::NewGuid().ToString('N')
$fixtures = Join-Path $root ('artifacts\diagnostics\'+$prefix)
& git -C $root check-ignore -q -- ('artifacts/diagnostics/'+$prefix+'/fixture.zip')
if ($LASTEXITCODE -ne 0) { throw 'Test fixtures must be ignored' }
New-Item -ItemType Directory -Path $fixtures | Out-Null
function Assert($Condition,$Message) { if (!$Condition) { throw $Message } }
# Load only the read-only baseline function, without executing preparation.
$parseErrors=$null; $parseTokens=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'prepare-portable-diagnostic.ps1'),[ref]$parseTokens,[ref]$parseErrors)
Assert (!$parseErrors.Count) 'Preparation script did not parse'
$baselineFunction=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Get-StableProfileBaseline'},$true)
Assert ($null -ne $baselineFunction) 'Missing stable baseline function'
Invoke-Expression $baselineFunction.Extent.Text
$baselineFixture=Join-Path $fixtures 'synthetic-profile.ini'
[IO.File]::WriteAllText($baselineFixture,'current fixture bytes')
$stable=Get-StableProfileBaseline $baselineFixture
Assert ($stable.sha256 -eq (Get-FileHash $baselineFixture -Algorithm SHA256).Hash) 'Stable baseline digest mismatch'
Assert ([Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($stable.bytes)) -ceq 'current fixture bytes') 'Stable baseline retained wrong bytes'
$mismatchRejected=$false
try { Get-StableProfileBaseline $baselineFixture -ReadBytes { param($File) [Text.Encoding]::UTF8.GetBytes('earlier fixture bytes') } | Out-Null } catch { $mismatchRejected=$true }
Assert $mismatchRejected 'Inconsistent retained bytes and live hash were accepted'
Assert ((Get-StableProfileBaseline $baselineFixture).sha256 -eq $stable.sha256) 'Stable baseline did not recover after mismatch'
Write-Host 'PASS stable baseline and inconsistent acquisition rejection'
function New-Package($Name,$Extra) {
    $file = Join-Path $fixtures ($Name+'.zip')
    $zip = [IO.Compression.ZipFile]::Open($file,[IO.Compression.ZipArchiveMode]::Create)
    try {
        $records = @{'lib/net45/TrafficMonitor.exe'='synthetic executable bytes'; 'lib/net45/installed-mode.ini'="[installation]`r`nmanaged=true`r`n"; 'lib/net45/global_cfg.ini'='unsafe old flags'; 'lib/net45/native-interface.ini'='old preferences'; 'Update.exe'='outside runtime, omitted'}
        foreach ($key in $Extra.Keys) { $records[$key]=$Extra[$key] }
        foreach ($key in $records.Keys) { $entry=$zip.CreateEntry($key); $stream=$entry.Open(); try { $bytes=[Text.Encoding]::ASCII.GetBytes($records[$key]);$stream.Write($bytes,0,$bytes.Length) } finally {$stream.Dispose()} }
    } finally {$zip.Dispose()}
    return $file
}
$package = New-Package 'valid' @{}
$packageHash = (Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash
$sha = [Security.Cryptography.SHA256]::Create()
try { $exeHash = ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::ASCII.GetBytes('synthetic executable bytes')))).Replace('-','') } finally { $sha.Dispose() }
$script:cases = 1
function Invoke-Case($Label,$Zip,$Fixture,$PackageDigest,$ExecutableDigest,$Commit,$Success) {
    $result = & powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'prepare-portable-diagnostic.ps1') -PackagePath $Zip -ExpectedPackageSHA256 $PackageDigest -ExpectedExecutableSHA256 $ExecutableDigest -SourceCommit $Commit -FixtureName $Fixture
    $code=$LASTEXITCODE
    $json = $result | ConvertFrom-Json
    Assert ($json.prepared -eq $Success) ($Label+': wrong verdict')
    Assert ($json.launched -eq $false) ($Label+': launch was claimed')
    Assert (($code -eq 0) -eq $Success) ($Label+': wrong exit status')
    Assert (@($json.PSObject.Properties.Name | Where-Object { $_ -notin @('prepared','launched','stage','fixture') }).Count -eq 0) ($Label+': stdout exposed private fields')
    Write-Host ('PASS '+$Label);$script:cases++
}
$validName=$prefix+'-valid'
Invoke-Case 'valid preparation' $package $validName $packageHash $exeHash $source $true
$output=Join-Path $root ('artifacts\diagnostics\'+$validName)
$runtime=Join-Path $output 'runtime'
Assert (!(Test-Path (Join-Path $runtime 'installed-mode.ini'))) 'Managed marker was not omitted'
Assert (!(Get-ChildItem $runtime -Recurse -Filter Update.exe)) 'Updater was extracted'
Assert ([IO.File]::ReadAllText((Join-Path $runtime 'global_cfg.ini')) -ceq "[config]`r`nportable_mode=true`r`n") 'Portable flags are not exact CRLF'
Assert ([IO.File]::ReadAllText((Join-Path $runtime 'native-interface.ini')) -ceq "[interface]`r`nschemaVersion=1`r`nlanguage=0`r`nappearance=1`r`n") 'Fresh English/light preferences absent'
$receipt=Get-Content -Raw (Join-Path $output 'receipt.json') | ConvertFrom-Json
Assert ($receipt.prepared -and !$receipt.launched -and $receipt.sourceCommit -eq $source) 'Private preparation receipt is incomplete'
$receiptBytes=[IO.File]::ReadAllBytes((Join-Path $output 'receipt.json'))
Assert ($receiptBytes[0] -eq [byte][char]'{') 'Receipt must begin directly with JSON, without a UTF-8 BOM'
Assert ($receipt.limitations.Contains('association of this package with that source requires an independently verified build receipt')) 'Source/package association limitation is missing'
foreach ($file in $receipt.normalProfile) { Assert ((Get-FileHash -LiteralPath $file.path -Algorithm SHA256).Hash -eq $file.sha256) 'Normal-profile baseline changed during preparation' }
$runKey=[Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Software\Microsoft\Windows\CurrentVersion\Run')
try {
    $present=$runKey -and ($runKey.GetValueNames() -contains 'TrafficMonitor')
    Assert ($receipt.startup.present -eq [bool]$present) 'Startup presence changed during preparation'
    if ($present) { Assert ($receipt.startup.value -ceq $runKey.GetValue('TrafficMonitor',$null,[Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)) 'Startup value changed during preparation' }
} finally { if ($runKey) {$runKey.Dispose()} }
$receiptHash=(Get-FileHash (Join-Path $output 'receipt.json')).Hash
Invoke-Case 'existing destination rejected' $package $validName $packageHash $exeHash $source $false
Assert ((Get-FileHash (Join-Path $output 'receipt.json')).Hash -eq $receiptHash) 'Existing destination was overwritten'
Invoke-Case 'bad package hash' $package ($prefix+'-bad-package') ('0'*64) $exeHash $source $false
Invoke-Case 'bad executable hash' $package ($prefix+'-bad-exe') $packageHash ('0'*64) $source $false
Invoke-Case 'unsafe fixture traversal' $package '../escape' $packageHash $exeHash $source $false
Invoke-Case 'absolute fixture rejected' $package 'C:\escape' $packageHash $exeHash $source $false
Invoke-Case 'missing source rejected' $package ($prefix+'-missing-source') $packageHash $exeHash ('0'*40) $false
Invoke-Case 'malformed source rejected' $package ($prefix+'-bad-source') $packageHash $exeHash 'HEAD' $false
foreach ($case in @(@{name='traversal';entry='lib/net45/../../escaped.txt'},@{name='updater';entry='lib/net45/Update.exe'},@{name='nested-updater';entry='lib/net45/sub/UPDATE.EXE'},@{name='drive-path';entry='lib/net45/C:/bad'},@{name='trailing-dot';entry='lib/net45/bad.'})) {
    $extra=@{};$extra[$case.entry]='rejected fixture'
    $zip=New-Package $case.name $extra
    Invoke-Case $case.name $zip ($prefix+'-'+$case.name) (Get-FileHash $zip -Algorithm SHA256).Hash $exeHash $source $false
}
$linked=New-Package 'archive-link' @{'lib/net45/link'='link target'}
$zip=[IO.Compression.ZipFile]::Open($linked,[IO.Compression.ZipArchiveMode]::Update)
try { $zip.GetEntry('lib/net45/link').ExternalAttributes = -1610612736 } finally { $zip.Dispose() }
Invoke-Case 'archive symlink rejected' $linked ($prefix+'-link') (Get-FileHash $linked -Algorithm SHA256).Hash $exeHash $source $false
Assert ((Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash -eq $packageHash) 'Original package changed'
Write-Host ('Portable preparation checks: '+$script:cases+' passed. Synthetic fixtures remain ignored; no application was launched.')

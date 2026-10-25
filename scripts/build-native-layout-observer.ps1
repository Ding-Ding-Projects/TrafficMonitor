$ErrorActionPreference='Stop'
try {
    $root=Split-Path $PSScriptRoot -Parent
    foreach($helper in @('native-interface-inventory.mjs','native-render-observation-check.mjs','test-native-render-join.mjs')) {
        & node (Join-Path $root ('tools/'+$helper)) --self-test
        if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    }
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if(!(Test-Path -LiteralPath $vswhere)){throw 'Existing Visual Studio discovery unavailable; no installation attempted.'}
    $installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if(!$installation){throw 'Existing MSVC unavailable; no installation attempted.'}
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $output=Join-Path $root ('artifacts/native-layout-observer-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $output | Out-Null
    $source=Join-Path $root 'tools/native-layout-observer.cpp'
    $sourceHash=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $inputHashes=@{}
    foreach($relative in @('tools/native-layout-observer.cpp','tools/native-layout-render-reader.h','TrafficMonitor/NativeMaterial/LayoutObservationProtocol.h','tools/native-interface-inventory.mjs','tools/native-render-observation-check.mjs','tools/inspect-native-render-join.mjs','tools/test-native-render-join.mjs')) {
        $inputHashes[$relative]=(Get-FileHash -LiteralPath (Join-Path $root $relative) -Algorithm SHA256).Hash
    }
    $exe=Join-Path $output 'native-layout-observer.exe'
    $command='call "'+$vcvars+'" >nul && cl.exe /nologo /std:c++17 /EHsc /W4 /utf-8 /MT /D_UNICODE /DUNICODE "'+$source+'" /Fe:"'+$exe+'" /link User32.lib BCrypt.lib Advapi32.lib'
    Push-Location $output
    try { & cmd.exe /d /c $command; $result=$LASTEXITCODE } finally {Pop-Location}
    if($result -ne 0){exit $result}
    # No product process is launched. These checks use unshown, owned native windows.
    $test=New-Object System.Diagnostics.Process
    $test.StartInfo.FileName=$exe
    $test.StartInfo.Arguments='--self-test'
    $test.StartInfo.UseShellExecute=$false
    $test.StartInfo.CreateNoWindow=$true
    $test.StartInfo.RedirectStandardOutput=$true
    $test.StartInfo.RedirectStandardError=$true
    try {
        if(!$test.Start()){throw 'Owned observer checks did not start.'}
        if(!$test.WaitForExit(15000)){$test.Kill();$test.WaitForExit();throw 'Owned observer checks timed out.'}
        $testOutput=$test.StandardOutput.ReadToEnd()
        $testError=$test.StandardError.ReadToEnd()
        $testExit=$test.ExitCode
    } finally {$test.Dispose()}
    $testOutput | Set-Content -LiteralPath (Join-Path $output 'checks.txt') -Encoding UTF8
    $testError | Set-Content -LiteralPath (Join-Path $output 'checks-error.txt') -Encoding UTF8
    Write-Host $testOutput
    if($testExit -ne 0){Write-Host $testError;exit $testExit}
    $observations=@(Get-ChildItem -LiteralPath $output -Filter 'selftest-*.jsonl')
    if($observations.Count -ne 1){throw 'Expected one retained structural observation.'}
    $rows=@(Get-Content -LiteralPath $observations[0].FullName | ForEach-Object {$_ | ConvertFrom-Json})
    $completed=@($rows | Where-Object phase -eq 'complete')
    if($completed.Count -ne 1 -or $completed[0].receiptComplete -ne $false -or $completed[0].validV2Receipt -ne $false -or $completed[0].validV3Receipt -ne $false){throw 'Structural output incorrectly claims complete evidence.'}
    if(@($rows | Where-Object { $_.phase -eq 'control' -and $_.className -eq 'Edit' -and $_.editFormattingArea.status -eq 'observed' }).Count -ne 1){throw 'Native EDIT formatting observation missing.'}
    if(@($rows | Where-Object phase -eq 'identity.start.verified').Count -ne 1 -or @($rows | Where-Object phase -eq 'identity.end.verified').Count -ne 1){throw 'Identity interval missing.'}
    $acl=Get-Acl -LiteralPath $observations[0].FullName
    $currentSid=[Security.Principal.WindowsIdentity]::GetCurrent().User.Value
    if(!$acl.AreAccessRulesProtected -or @($acl.Access | Where-Object {$_.IdentityReference.Translate([Security.Principal.SecurityIdentifier]).Value -ne $currentSid}).Count){throw 'Observation file permissions are not limited to the current identity.'}
    if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $sourceHash){throw 'Source changed during compilation/checks.'}
    foreach($relative in $inputHashes.Keys) {
        if((Get-FileHash -LiteralPath (Join-Path $root $relative) -Algorithm SHA256).Hash -ne $inputHashes[$relative]){throw 'An observer input changed during compilation/checks.'}
    }
    $receipt=@{source='tools/native-layout-observer.cpp';sourceSha256=$sourceHash;inputHashes=$inputHashes;executable=$exe;executableSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;entrypoint='build.bat --layout-observer';checks='passed';productRuntimeVerified=$false;receiptComplete=$false}
    $receipt | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'build-receipt.json') -Encoding UTF8
    $receipt | ConvertTo-Json -Compress
    exit 0
}catch{Write-Error $_;exit 1}

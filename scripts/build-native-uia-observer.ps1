$ErrorActionPreference='Stop'
try {
    $root=Split-Path $PSScriptRoot -Parent
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if(!(Test-Path -LiteralPath $vswhere)){throw 'Existing Visual Studio discovery is unavailable; no installation was attempted.'}
    $installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if(!$installation){throw 'Existing MSVC is unavailable; no installation was attempted.'}
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $output=Join-Path $root ('artifacts/native-uia-observer-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $output | Out-Null
    $source=Join-Path $root 'tools/native-uia-observer.cpp'
    $hash=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $exe=Join-Path $output 'native-uia-observer.exe'
    $command='call "'+$vcvars+'" >nul && cl.exe /nologo /std:c++17 /EHsc /W4 /utf-8 /MT /D_UNICODE /DUNICODE "'+$source+'" /Fe:"'+$exe+'" /link Ole32.lib OleAut32.lib User32.lib BCrypt.lib UIAutomationCore.lib'
    Push-Location $output
    try { & cmd.exe /d /c $command; $result=$LASTEXITCODE } finally { Pop-Location }
    if($result -ne 0){exit $result}
    if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $hash){throw 'Observer source changed during compilation.'}
    $receipt=@{source='tools/native-uia-observer.cpp';sourceSha256=$hash;executable=$exe;executableSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;entrypoint='build.bat --uia-observer';runtimeVerified=$false}
    $receipt | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'build-receipt.json') -Encoding UTF8
    $receipt | ConvertTo-Json -Compress
    exit 0
} catch { Write-Error $_; exit 1 }

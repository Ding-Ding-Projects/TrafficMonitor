$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-settings-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $exe=Join-Path $work 'DraftValidationTests.exe'
    $source=Join-Path $RepoRoot 'TrafficMonitor/NativeSettings/DraftValidationTests.cpp'
    $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /W4 /WX "'+$source+'" /Fe:"'+$exe+'"'
    Push-Location $work
    try { & cmd.exe /d /c $command; $compileExit=$LASTEXITCODE } finally { Pop-Location }
    if($compileExit -ne 0){exit $compileExit}
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Recreate the previous ownership defect only in disposable test inputs.
    # The repository header and the production executable remain unchanged.
    $header=Join-Path $RepoRoot 'TrafficMonitor/NativeSettings/DraftValidation.h'
    $original=[IO.File]::ReadAllText($header)
    $needle='return alreadyOpen?m_generation:++m_generation;'
    if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw 'Expected exactly one editor ownership expression'}
    $mutation=$original.Replace($needle,'(void)alreadyOpen;return ++m_generation;')
    [IO.File]::WriteAllText((Join-Path $work 'DraftValidation.h'),$mutation)
    $mutantSource=Join-Path $work 'DraftValidationTests.cpp'
    Copy-Item -LiteralPath $source -Destination $mutantSource
    $mutantExe=Join-Path $work 'MutatedBindingTests.exe'
    $mutantCommand='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /utf-8 /W4 /WX "'+$mutantSource+'" /Fe:"'+$mutantExe+'"'
    Push-Location $work
    try { & cmd.exe /d /c $mutantCommand; $mutationCompileExit=$LASTEXITCODE } finally { Pop-Location }
    if($mutationCompileExit -ne 0){exit $mutationCompileExit}
    $mutationLog=Join-Path $work 'mutation-output.txt'
    & cmd.exe /d /c ('"'+$mutantExe+'" > "'+$mutationLog+'" 2>&1')
    $mutationExit=$LASTEXITCODE
    $mutationOutput=Get-Content -LiteralPath $mutationLog
    $mutationOutput | ForEach-Object { Write-Host $_ }
    if($mutationExit -ne 1 -or ($mutationOutput -join "`n") -notmatch 'repeated focus preserves original callback binding'){throw 'Ownership mutation was not rejected by the expected regression'}
    if([IO.File]::ReadAllText($header) -cne $original){throw 'Production header changed during mutation verification'}
    Write-Host 'Expected rejection: disposable repeated-open ownership mutation returned 1.'
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

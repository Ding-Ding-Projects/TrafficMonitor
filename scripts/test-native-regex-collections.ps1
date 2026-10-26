$ErrorActionPreference='Stop'
. "$PSScriptRoot/build-tools.ps1"
try {
    $installation=Get-BuildInstallation
    $version=Get-Content -LiteralPath (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')
    $compiler=Join-Path $installation "VC/Tools/MSVC/$($version.Trim())/bin/Hostx64/x64/cl.exe"
    $vcvars=Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $work=Join-Path $RepoRoot ('artifacts/native-regex-collections-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $work | Out-Null
    $regex=Join-Path $RepoRoot 'TrafficMonitor/NativeRegex'
    $include=Join-Path $RepoRoot 'include'
    function Build-Tests($collections,$exe) {
        $command='call "'+$vcvars+'" >nul && "'+$compiler+'" /nologo /std:c++17 /EHsc /MD /D_AFXDLL /D_UNICODE /DUNICODE /DWITHOUT_TEMPERATURE /I"'+$include+'" "'+(Join-Path $regex 'CollectionsTests.cpp')+'" "'+$collections+'" /Fe:"'+$exe+'"'
        Push-Location $work
        try { & cmd.exe /d /c $command | Out-Host; return [int]$LASTEXITCODE } finally { Pop-Location }
    }
    $exe=Join-Path $work 'CollectionsTests.exe'
    $compileExit=Build-Tests (Join-Path $regex 'Collections.cpp') $exe
    if($compileExit -ne 0){exit $compileExit}
    & $exe
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    # Mutation checks: each deliberate production break must be caught by its expected regression.
    $source=Join-Path $regex 'Collections.cpp'
    $original=[IO.File]::ReadAllText($source)
    function Test-Mutant($label,$needle,$replacement,$expected) {
        if(($original.Split([string[]]@($needle),[StringSplitOptions]::None)).Count -ne 2){throw "Expected exactly one site for the $label mutation"}
        # Keep relative includes resolvable by compiling the mutant beside the originals.
        $mutantInSource=Join-Path $regex ('MutatedCollections-'+[guid]::NewGuid().ToString('N')+'.cpp')
        try {
            [IO.File]::WriteAllText($mutantInSource,$original.Replace($needle,$replacement))
            $mutant=Join-Path $work ('Mutated-'+[guid]::NewGuid().ToString('N')+'.exe')
            $compileExit=Build-Tests $mutantInSource $mutant
        } finally { Remove-Item -LiteralPath $mutantInSource -Force -ErrorAction SilentlyContinue }
        if($compileExit -ne 0){exit $compileExit}
        $output=& $mutant
        $result=$LASTEXITCODE
        $output | ForEach-Object { Write-Host $_ }
        if($result -ne 1 -or ($output -join "`n") -notmatch $expected){throw "The $label mutation was not rejected by the expected regression"}
    }
    # A merge that overwrites a conflicting record must be caught.
    Test-Mutant 'conflict' 'if(*it==item)continue;throw' 'if(true)continue;throw' 'FAIL merge conflict rejected'
    # A decoder that discards the stored description must be caught.
    Test-Mutant 'description decode' 'if(schema>=2)item.description=r.Text(DescriptionLimit);' 'if(schema>=2)r.Text(DescriptionLimit);' 'FAIL description persists on disk'
    if([IO.File]::ReadAllText($source) -cne $original){throw 'Production source changed during mutation verification'}
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

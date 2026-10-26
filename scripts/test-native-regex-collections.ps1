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
    # Stock-control scan: the regex workbench must stay on Material components. System message boxes,
    # combo boxes and button-class check or radio styles are rejected in every NativeRegex source.
    $stockPattern='MessageBox[AW]?|AfxMessageBox|CComboBox|CButton|BS_\w*(CHECK|RADIO)\w*|L"(BUTTON|COMBOBOX)"'
    function Find-StockControls($directory) {
        $found=@()
        $files=@(Get-ChildItem -LiteralPath $directory -Recurse -File | Where-Object { $_.Extension -in '.cpp','.h' })
        if($files.Count -eq 0){throw "No sources found to scan in $directory"}
        foreach($file in $files){
            $number=0
            foreach($line in [IO.File]::ReadAllLines($file.FullName)){
                $number++
                $code=$line -replace '//.*$',''
                if($code -match $stockPattern){$found+="$($file.Name):${number}: $($line.Trim())"}
            }
        }
        return ,$found
    }
    $stock=Find-StockControls $regex
    if($stock.Count -ne 0){$stock | ForEach-Object { Write-Host $_ };throw 'Stock controls found in NativeRegex'}
    Write-Host 'PASS no stock message box, combo box or check/radio button in NativeRegex'
    # Mutation: each forbidden construct injected into a copy of the sources must be rejected.
    foreach($mutation in @('MessageBoxW(nullptr,L"x",L"y",MB_OK);','CComboBox stockCombo;','CButton stockCheck;','DWORD stockStyle=BS_AUTOCHECKBOX;')){
        $copy=Join-Path $work ('scan-'+[guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $copy | Out-Null
        Copy-Item -Path (Join-Path $regex '*') -Destination $copy -Recurse
        Add-Content -LiteralPath (Join-Path $copy 'Workbench.cpp') -Value $mutation
        $hits=@(Find-StockControls $copy)
        Write-Host "scan of mutated copy reports $($hits.Count) finding(s)"
        if($hits.Count -eq 0){throw "The stock-control scan did not reject: $mutation"}
    }
    Write-Host 'PASS stock-control scan rejects injected MessageBox, CComboBox, CButton and BS_ check styles'
    & $exe
    exit $LASTEXITCODE
} catch { Write-Error $_; exit 1 }

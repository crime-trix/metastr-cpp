param(
    [string] $BuildDir = "build-bench",
    [string] $Config = "Release",
    [string] $Needle = "api.secret.endpoint"
)

$ErrorActionPreference = "Stop"

function Resolve-CMake {
    $command = Get-Command cmake -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $vsCmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (Test-Path $vsCmake) {
        return $vsCmake
    }

    throw "cmake was not found in PATH or the default Visual Studio install path"
}

$cmake = Resolve-CMake

& $cmake -S . -B $BuildDir -DMETASTR_BUILD_BENCHMARKS=ON -DMETASTR_BUILD_EXAMPLES=OFF -DMETASTR_BUILD_TESTS=OFF
if ($LASTEXITCODE -ne 0) {
    throw "benchmark configure failed with exit code $LASTEXITCODE"
}

& $cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    throw "benchmark build failed with exit code $LASTEXITCODE"
}

$programs = @(
    @{ Name = "baseline"; Path = Join-Path $BuildDir "$Config/metastr-bench-baseline.exe" },
    @{ Name = "metastr_stream"; Path = Join-Path $BuildDir "$Config/metastr-bench-stream.exe" },
    @{ Name = "metastr_auto"; Path = Join-Path $BuildDir "$Config/metastr-bench-auto.exe" }
)

function Count-AsciiNeedle([string] $Path, [string] $Needle) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes)
    $count = 0
    $offset = 0

    while ($true) {
        $index = $text.IndexOf($Needle, $offset, [System.StringComparison]::Ordinal)
        if ($index -lt 0) {
            return $count
        }

        ++$count
        $offset = $index + $Needle.Length
    }
}

$baselineSize = $null
$rows = foreach ($program in $programs) {
    $process = Start-Process -FilePath $program.Path -NoNewWindow -Wait -PassThru
    $exitCode = $process.ExitCode
    $size = (Get-Item $program.Path).Length
    if ($null -eq $baselineSize) {
        $baselineSize = $size
    }

    [pscustomobject]@{
        Program = $program.Name
        SizeBytes = $size
        DeltaBytes = $size - $baselineSize
        PlaintextHits = Count-AsciiNeedle $program.Path $Needle
        ExitCode = $exitCode
    }
}

$rows | Format-Table -AutoSize

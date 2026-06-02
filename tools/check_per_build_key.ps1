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
$first = "build-key-a"
$second = "build-key-b"

Remove-Item -LiteralPath $first, $second -Recurse -Force -ErrorAction SilentlyContinue

& $cmake -S . -B $first -DMETASTR_BUILD_EXAMPLES=OFF -DMETASTR_BUILD_TESTS=OFF
if ($LASTEXITCODE -ne 0) {
    throw "first configure failed with exit code $LASTEXITCODE"
}

& $cmake -S . -B $second -DMETASTR_BUILD_EXAMPLES=OFF -DMETASTR_BUILD_TESTS=OFF
if ($LASTEXITCODE -ne 0) {
    throw "second configure failed with exit code $LASTEXITCODE"
}

$firstConfig = Join-Path $first "generated/metastr/metastr_build_config.hpp"
$secondConfig = Join-Path $second "generated/metastr/metastr_build_config.hpp"
$firstHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $firstConfig).Hash
$secondHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $secondConfig).Hash

if ($firstHash -eq $secondHash) {
    throw "generated metastr build keys are identical"
}

Write-Host "Generated build keys differ: $firstHash vs $secondHash"

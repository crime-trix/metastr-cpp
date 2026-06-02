param(
    [Parameter(Mandatory = $true)]
    [string] $Output
)

$ErrorActionPreference = "Stop"

$bytes = New-Object byte[] 32
$rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
try {
    $rng.GetBytes($bytes)
} finally {
    $rng.Dispose()
}

function Read-U64([byte[]] $Data, [int] $Offset) {
    $value = [BitConverter]::ToUInt64($Data, $Offset)
    return "0x{0:x16}ull" -f $value
}

$content = @"
#pragma once

#define METASTR_HAS_BUILD_KEY 1
#define METASTR_BUILD_KEY0 $(Read-U64 $bytes 0)
#define METASTR_BUILD_KEY1 $(Read-U64 $bytes 8)
#define METASTR_BUILD_KEY2 $(Read-U64 $bytes 16)
#define METASTR_BUILD_KEY3 $(Read-U64 $bytes 24)
"@

$directory = Split-Path -Parent $Output
if ($directory) {
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
}
Set-Content -LiteralPath $Output -Value $content -Encoding ascii

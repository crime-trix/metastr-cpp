param(
    [Parameter(Mandatory = $true)]
    [string] $Path,

    [Parameter(Mandatory = $true)]
    [string[]] $Needles
)

$bytes = [System.IO.File]::ReadAllBytes($Path)
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes)

foreach ($needle in $Needles) {
    if ($ascii.Contains($needle)) {
        Write-Error "Plaintext string found in binary: $needle"
        exit 1
    }
}

Write-Host "No protected plaintext strings found in $Path"

param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath
)

$current = [Environment]::GetEnvironmentVariable('Path', 'User')
if ($current -split ';' | Where-Object { $_ -ieq $TargetPath }) {
    exit 0
}
exit 1

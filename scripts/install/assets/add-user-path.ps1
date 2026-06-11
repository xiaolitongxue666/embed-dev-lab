param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath
)

$current = [Environment]::GetEnvironmentVariable('Path', 'User')
if ([string]::IsNullOrEmpty($current)) {
    $current = ''
}

$parts = $current -split ';' | Where-Object { $_ -ne '' }
if ($parts -contains $TargetPath) {
    exit 0
}

$newPath = if ($current) { "$current;$TargetPath" } else { $TargetPath }
[Environment]::SetEnvironmentVariable('Path', $newPath, 'User')

# Build the driver and the applications

[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [string]$Arch = "",
    [switch]$NoPause = $false
)

# Default architecture is local architecture, x64 or ARM64
if (-not $Arch) {
    $Arch = if ((Get-CimInstance Win32_OperatingSystem).OSArchitecture -match "arm") {"AMR64"} else {"x64"}
}

# Find MSBuild
Write-Output "Searching MSBuild..."
$MSRoots = @("C:\Program Files*\MSBuild", "C:\Program Files*\Microsoft Visual Studio")
$MSBuild = Get-ChildItem -Recurse -Path $MSRoots -Include MSBuild.exe -ErrorAction Ignore |
           ForEach-Object { $_.FullName} |
           Select-Object -Last 1

if (-not $MSBuild) {
    Write-Output "MSBuild not found"
}
else {
    # Build the project.
    Write-Output "Building for $Arch..."
    $SolutionFile = "$PSScriptRoot\windriver-test.sln"
    & $MSBuild $SolutionFile /nologo /property:Configuration=Release /property:Platform=$Arch
}

if (-not $NoPause) {
    pause
}

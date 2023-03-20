# Build the driver and the applications for ARM64 and package an archive.

[CmdletBinding(SupportsShouldProcess=$true)]
param([switch]$NoPause = $false)

# Build driver and applications.
& "$PSScriptRoot\build.ps1" -Arch ARM64 -NoPause

# Build archive.
Compress-Archive -DestinationPath "$PSScriptRoot\windriver-test-arm64.zip" @(
    "$PSScriptRoot\driver\loader.ps1",
    "$PSScriptRoot\driver\loader-unload.ps1",
    "$PSScriptRoot\driver\ARM64\Release\loader.exe",
    "$PSScriptRoot\driver\ARM64\Release\cpusysregs.sys",
    "$PSScriptRoot\app\ARM64\Release\testapp.exe"    
)

if (-not $NoPause) {
    pause
}

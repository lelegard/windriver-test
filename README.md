# Sample Windows kernel driver with buffered ioctl

## Prerequisites

- Install the Windows Driver Kit (WDK).
  - See: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
  - May need to install the latest Windows SDK first (link from same page).

- Disable driver signature validation.
  - Start Menu -> Settings -> System -> Recovery -> Advanced Startup -> Restart now
  - "Light blue" boot menu -> Troubleshoot -> Advanced options -> Startup settings -> Restart
  - Press F7 (for 7th option "Disable driver signature enforcement")
  - This settings may not be persistent across reboots (to be confirmed).

- Disable secure boot on Windows 11
  - Start Menu -> Settings -> System -> Recovery -> Advanced Startup -> Restart now
  - "Light blue" boot menu -> Troubleshoot -> UEFI Firmware Settings.
  - Find the Secure Boot setting in your BIOS menu. If possible, set it to Disabled.
    This option is usually in either the Security tab, the Boot tab, or the Authentication tab.
    Save changes and exit => reboot

## Loading / unloading the driver

The driver must be explicitly loaded and unloaded. This must be done from and administrator
account. There are two different ways to do this:

- Using a PowerShell script. See `driver\loader.ps1`
- Using a C/C++ application. See `driver\loader.cpp`

Once the driver has been loaded using an administrator account, the application
`app\testapp.cpp` can be run from any account.

## References

See other Windows drivers samples in https://github.com/microsoft/Windows-driver-samples/

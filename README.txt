Disabling secure boot:
Start Menu -> Settings -> System -> Recovery -> Advanced Startup -> Restart now
"Light blue" boot menu -> Troubleshoot -> UEFI Firmware Settings.
Find the Secure Boot setting in your BIOS menu. If possible, set it to Disabled.
This option is usually in either the Security tab, the Boot tab, or the Authentication tab.
Save changes and exit => reboot

Disabling driver signature validation:
Start Menu -> Settings -> System -> Recovery -> Advanced Startup -> Restart now
"Light blue" boot menu -> Troubleshoot -> Advanced options -> Startup settings -> Restart
F7 (for 7th option "Disable driver signature enforcement")

Get devcon.exe from C:\Program Files (x86)\Windows Kits\10\Tools\10.0.22621.0\x64
Get driver from cpusysregs\x64\Release\cpusysregs
- cpusysregs.cat
- cpusysregs.inf
- cpusysregs.sys

> .\devcon.exe install cpusysregs.inf Root\cpusysregs
Device node created. Install is complete when drivers are installed...
Updating drivers for Root\cpusysregs from C:\....\cpusysregs\cpusysregs.inf.
Drivers installed successfully.

In case of error, see logs in C:\Windows\INF\setupapi.dev.log

See samples in https://github.com/microsoft/Windows-driver-samples/
https://github.com/microsoft/Windows-driver-samples/tree/main/general/ioctl/kmdf

Get the wdfcoinstaller.msi package from [WDK 8 Redistributable Components](https://go.microsoft.com/fwlink/p/?LinkID=253170).
This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation.
You will see no confirmation that the installation has completed. You can verify that the redistributables
have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory
of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.
==> this is the Intel version.
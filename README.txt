Install the Windows Driver Kit (WDK).
May need to install the latest Windows SDK (same page).
https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

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


See samples in https://github.com/microsoft/Windows-driver-samples/
https://github.com/microsoft/Windows-driver-samples/tree/main/general/ioctl/kmdf

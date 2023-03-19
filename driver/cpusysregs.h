// Device type in the "User Defined" range.
#define SIOCTL_TYPE 40000

// The IOCTL function codes (12 bits) from 0x800 to 0xFFF are for customer use.
#define IOCTL_SIOCTL_METHOD_BUFFERED CTL_CODE(SIOCTL_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CSR_DRIVER_NAME      "cpusysregs"
#define CSR_DEVICE_NAME      "\\\\.\\cpusysregs"
#define CSR_NT_DEVICE_NAME   L"\\Device\\cpusysregs"
#define CSR_DOS_DEVICE_NAME  L"\\DosDevices\\cpusysregs"

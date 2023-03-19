#if !defined(_CPUSYSREG_H)
#define _CPUSYSREG_H 1

#if defined(KERNEL)
#include <ntddk.h>
#else
#include <winioctl.h>
#endif

#if defined(__cplusplus)
#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif
#endif // __cplusplus

// Device type in the "User Defined" range.
#define SIOCTL_TYPE 40000

// The IOCTL function codes (12 bits) from 0x800 to 0xFFF are for customer use.
#define CSR_IOCTL_FOO CTL_CODE(SIOCTL_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

// The command FOO uses this structure as parameter.
typedef struct {
    int in;
    int out;
} csr_foo_t;

#define CSR_DRIVER_NAME      "cpusysregs"
#define CSR_DEVICE_NAME      "\\\\.\\cpusysregs"
#define CSR_NT_DEVICE_NAME   L"\\Device\\cpusysregs"
#define CSR_DOS_DEVICE_NAME  L"\\DosDevices\\cpusysregs"

#endif // _CPUSYSREG_H

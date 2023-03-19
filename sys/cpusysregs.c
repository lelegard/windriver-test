#include <ntddk.h>
#include <string.h>
#include "cpusysregs.h"

#define TEST_TEXT "(50) This String is from Device Driver"

// Device driver routine declarations.

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD csr_unload;
_Dispatch_type_(IRP_MJ_CREATE) _Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH csr_open_close;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH csr_ioctl;

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, csr_open_close)
#pragma alloc_text(PAGE, csr_ioctl)
#pragma alloc_text(PAGE, csr_unload)
#endif

// Driver initialization.

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
    NTSTATUS       status;
    UNICODE_STRING device_name;
    UNICODE_STRING link_name;
    PDEVICE_OBJECT device_object = NULL;

    UNREFERENCED_PARAMETER(registry_path);

    // Driver entry point in the driver object.
    driver_object->MajorFunction[IRP_MJ_CREATE] = csr_open_close;
    driver_object->MajorFunction[IRP_MJ_CLOSE] = csr_open_close;
    driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = csr_ioctl;
    driver_object->DriverUnload = csr_unload;

    // Create our device.
    RtlInitUnicodeString(&device_name, CSR_NT_DEVICE_NAME);
    status = IoCreateDevice(driver_object,             // Our Driver Object
                            0,                        // No device extension
                            &device_name,             // Device name "\Device\xxxxx"
                            FILE_DEVICE_UNKNOWN,      // Device type
                            FILE_DEVICE_SECURE_OPEN,  // Device characteristics
                            FALSE,                    // Not an exclusive device
                            &device_object);          // Returned ptr to Device Object
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create a symbolic link between our device name and the Win32 name.
    RtlInitUnicodeString(&link_name, CSR_DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&link_name, &device_name);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(device_object);
    }

    return status;
}

// This routine is called by the I/O system to unload the driver.

void csr_unload(PDRIVER_OBJECT driver_object)
{
    PDEVICE_OBJECT device_object = driver_object->DeviceObject;
    UNICODE_STRING link_name;

    PAGED_CODE();

    // Delete the link from our device name to a name in the Win32 namespace.
    RtlInitUnicodeString(&link_name, CSR_DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&link_name);

    // Delete the device object.
    if (device_object != NULL) {
        IoDeleteDevice(device_object);
    }
}

// This routine is called by the I/O system when the SIOCTL is opened or closed.

NTSTATUS csr_open_close(PDEVICE_OBJECT device_object, PIRP irp)
{
    UNREFERENCED_PARAMETER(device_object);
    PAGED_CODE();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// This routine is called by the I / O system to perform a device I / O control function.

NTSTATUS csr_ioctl(PDEVICE_OBJECT device_object, PIRP irp)
{
    PIO_STACK_LOCATION irp_sp;// Pointer to current stack location
    NTSTATUS           status = STATUS_SUCCESS;// Assume success
    ULONG              inBufLength; // Input buffer length
    ULONG              outBufLength; // Output buffer length
    PCHAR              inBuf, outBuf; // pointer to Input and output buffer
    PCHAR              data = TEST_TEXT;
    size_t             datalen = strlen(data)+1; //Length of data including null

    UNREFERENCED_PARAMETER(device_object);

    PAGED_CODE();

    irp_sp = IoGetCurrentIrpStackLocation(irp);
    inBufLength = irp_sp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irp_sp->Parameters.DeviceIoControl.OutputBufferLength;

    if (inBufLength == 0 || outBufLength == 0) {
        status = STATUS_INVALID_PARAMETER;
    }
    else {
        switch (irp_sp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_SIOCTL_METHOD_BUFFERED:
            // In this method the I/O manager allocates a buffer large enough to
            // to accommodate larger of the user input buffer and output buffer,
            // assigns the address to Irp->AssociatedIrp.SystemBuffer, and
            // copies the content of the user input buffer into this SystemBuffer

            // Input buffer and output buffer is same in this case, read the
            // content of the buffer before writing to it
            inBuf = irp->AssociatedIrp.SystemBuffer;
            outBuf = irp->AssociatedIrp.SystemBuffer;

            // Write to the buffer over-writes the input buffer content
            RtlCopyMemory(outBuf, data, outBufLength);

            // Assign the length of the data copied to IoStatus.Information
            // of the Irp and complete the Irp.
            irp->IoStatus.Information = (outBufLength < datalen ? outBufLength : datalen);

            // When the Irp is completed the content of the SystemBuffer
            // is copied to the User output buffer and the SystemBuffer is freed.
            break;

        default:
            // The specified I/O control code is unrecognized by this driver.
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

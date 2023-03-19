#include <stdlib.h>
#include <string>
#include <iostream>
#include <algorithm>
#include "cpusysregs.h"

// Format a string from a Windows status code.
std::string ErrorMessage(::DWORD code = ::GetLastError())
{
    std::string message;
    message.resize(1024);
    size_t length = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code, 0, &message[0], ::DWORD(message.size()), nullptr);
    message.resize(std::min(length, message.size()));
    if (message.empty()) {
        message.resize(1024);
        length = ::snprintf(&message[0], message.size(), "System error %d (0x%X)", int(code), int(code));
        message.resize(std::max<size_t>(0, length));
    }
    return message;
}

// Test application.
int main(int argc, const char* argv[])
{
    // Use command line parameter as integer to pass to the driver. Default: 47.
    const int param = argc > 1 ? std::atoi(argv[1]) : 47;

    const ::HANDLE device = ::CreateFileA(CSR_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                                          0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (device == INVALID_HANDLE_VALUE) {
        const ::DWORD err = ::GetLastError();
        std::cout << "Error opening " << CSR_DEVICE_NAME << ", " << ErrorMessage(err) << std::endl;
        return EXIT_FAILURE;
    }

    // Perform ioctl.
    csr_foo_t in_data = {param, 0};
    csr_foo_t out_data = {param, 0};
    ::ULONG retsize = 0;
    bool ok = ::DeviceIoControl(device, ::DWORD(CSR_IOCTL_FOO),
                                &in_data, ::DWORD(sizeof(in_data)),
                                &out_data, ::DWORD(sizeof(out_data)),
                                &retsize, NULL);

    if (ok) {
        printf("In data: {%d, %d}, out data: {%d, %d}, returned size: %d bytes\n\n",
               in_data.in, in_data.out, out_data.in, out_data.out, int(retsize));
    }
    else {
        const ::DWORD err = ::GetLastError();
        std::cout << "DeviceIoControl() error: " << ErrorMessage(err) << std::endl;
    }
    
    ::CloseHandle(device);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

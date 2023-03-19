#include <windows.h>
#include <winioctl.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <algorithm>

#include "cpusysregs.h"

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

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
    const ::HANDLE device =
        ::CreateFileA(CSR_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                      0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (device == INVALID_HANDLE_VALUE) {
        const ::DWORD err = ::GetLastError();
        std::cout << "Error opening " << CSR_DEVICE_NAME << ", " << ErrorMessage(err) << std::endl;
        return EXIT_FAILURE;
    }

    // Perform ioctl.
    char* input = "String from user application";
    char output[100];
    ::memset(output, 0, sizeof(output));

    ::ULONG retsize = 0;
    bool ok = ::DeviceIoControl(device, ::DWORD(IOCTL_SIOCTL_METHOD_BUFFERED),
                                input, ::DWORD(::strlen(input) + 1),
                                output, sizeof(output),
                                &retsize, NULL);

    if (ok) {
        printf("Returned text: \"%.*s\"\n\n", int(retsize), output);
    }
    else {
        const ::DWORD err = ::GetLastError();
        std::cout << "DeviceIoControl() error: " << ErrorMessage(err) << std::endl;
    }
    
    ::CloseHandle(device);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

#include <DriverSpecs.h>
#include <windows.h>

#pragma warning(disable:4201)  // nameless struct/union
#include <winioctl.h>
#pragma warning(default:4201)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <strsafe.h>
#include <cstring>
#include "public.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdarg>

#if defined(min)
#undef min
#endif

std::string Format(const char* fmt, ...)
{
    va_list ap;

    // Get required output size.
    va_start(ap, fmt);
    int length = ::vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    if (length < 0) {
        return std::string(); // error
    }

    // Actual formatting.
    std::string buf(length + 1, '\0');
    va_start(ap, fmt);
    length = ::vsnprintf(&buf[0], buf.size(), fmt, ap);
    va_end(ap);

    buf.resize(std::max<size_t>(0, length));
    return buf;
}

std::string ErrorMessage(::DWORD code = ::GetLastError())
{
    std::string message;
    message.resize(1024);
    size_t length = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code, 0, &message[0], ::DWORD(message.size()), nullptr);
    message.resize(std::min(length, message.size()));
    return message.empty() ? Format("System error %d (0x%X)", int(code), int(code)) : message;
}

bool InstallDriverFile(std::string& driver_path)
{
    // Driver file name.
    const std::string driver_basename(DRIVER_NAME ".sys");

    // Get current executable.
    std::string file(2048, ' ');
    size_t length = ::GetModuleFileNameA(nullptr, &file[0], ::DWORD(file.size()));
    file.resize(std::min(length, file.size()));

    // Extract directory part.
    size_t pos = file.rfind('\\');
    if (pos != std::string::npos) {
        file.resize(pos);
    }
    std::cout << "App dir: " << file << std::endl;

    // Check if driver is present in same directory as application.
    std::string driver_file(file + '\\' + driver_basename);
    if (::GetFileAttributesA(driver_file.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Not found, replace '/exe/' with '/sys/' in the directory.
        pos = driver_file.rfind("\\exe\\");
        if (pos != std::string::npos) {
            driver_file.replace(pos + 1, 3, "sys");
        }
    }

    // Check if driver file exists.
    if (::GetFileAttributesA(driver_file.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cout << "Driver file not found: " << driver_file << std::endl;
        return false;
    }
    std::cout << "Driver: " << driver_file << std::endl;

    // Get Windows directory (typically C:\Windows)
    driver_path.resize(MAX_PATH);
    length = ::GetWindowsDirectoryA(&driver_path[0], MAX_PATH);
    driver_path.resize(std::min<size_t>(length, MAX_PATH));

    // Build final driver location.
    driver_path += "\\System32\\Drivers\\";
    driver_path += driver_basename;

    // Install the driver in system area.
    if (!::CopyFileA(driver_file.c_str(), driver_path.c_str(), false)) {
        const ::DWORD err = ::GetLastError();
        std::cout << "CopyFile() error: " << ErrorMessage(err) << ", destination: " << driver_path << std::endl;
        return false;
    }

    // Connect to the Service Control Manager and open the Services database.
    const ::SC_HANDLE schSCManager = ::OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenSCManagerA() error: " << ErrorMessage(err) << std::endl;
        return false;
    }

    // Create a new a service object.
    std::cout << "CreateService(\"" << DRIVER_NAME << "\", \"" << driver_path << "\")" << std::endl;
    const ::SC_HANDLE schService = ::CreateServiceA(schSCManager,
        DRIVER_NAME,            // address of name of service to start
        DRIVER_NAME,            // address of display name
        SERVICE_ALL_ACCESS,     // type of access to service
        SERVICE_KERNEL_DRIVER,  // type of service
        SERVICE_DEMAND_START,   // when to start service
        SERVICE_ERROR_NORMAL,   // severity if service fails to start
        driver_path.c_str(),    // address of name of binary file
        nullptr,                // service does not belong to a group
        nullptr,                // no tag requested
        nullptr,                // no dependency names
        nullptr,                // use LocalSystem account
        nullptr);               // no password for service account

    bool ok = schService != nullptr;
    if (!ok) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            ok = true;
        }
        else {
            std::cout << "CreateService() error: " << ErrorMessage(err) << std::endl;
        }
    }
    else {
        // Start the execution of the service (i.e. start the driver).
        ok = ::StartServiceA(schService, 0, NULL);
        if (!ok) {
            const ::DWORD err = ::GetLastError();
            if (err == ERROR_SERVICE_ALREADY_RUNNING) {
                ok = true;
            }
            else {
                std::cout << "StartServiceA() error: " << ErrorMessage(err) << std::endl;
            }
        }
        // Close the service object.
        CloseServiceHandle(schService);
    }

    // Close handle to service control manager.
    ::CloseServiceHandle(schSCManager);
    return ok;
}

bool RemoveDriverFile(const std::string& driver_path)
{
    printf("RemoveDriverFile(\"%s\")\n", driver_path.c_str());

    // Connect to the Service Control Manager and open the Services database.
    const ::SC_HANDLE schSCManager = ::OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenSCManagerA() error: " << ErrorMessage(err) << std::endl;
        return false;
    }

    // Open the handle to the existing service.
    const ::SC_HANDLE schService = ::OpenServiceA(schSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
    bool ok = schService != nullptr;
    if (!ok) {
        printf("OpenServiceA() error: %d \n", ::GetLastError());
    }
    else {
        // Request that the service stop.
        ::SERVICE_STATUS serviceStatus;
        ok = ::ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
        if (!ok) {
            const ::DWORD err = ::GetLastError();
            std::cout << "ControlService() error: " << ErrorMessage(err) << std::endl;
        }
        else {
            // Mark the service for deletion from the service control manager database.
            ok = ::DeleteService(schService);
            if (!ok) {
                const ::DWORD err = ::GetLastError();
                std::cout << "DeleteService() error: " << ErrorMessage(err) << std::endl;
            }
        }
        // Close the service object.
        CloseServiceHandle(schService);
    }

    // Close handle to service control manager.
    ::CloseServiceHandle(schSCManager);
    return ok;
}


VOID DoIoctls(HANDLE hDevice)
{
    char OutputBuffer[100];
    char InputBuffer[200];
    BOOL bRc;
    ULONG bytesReturned;

    // Performing METHOD_BUFFERED
    if (FAILED(StringCchCopy(InputBuffer, sizeof(InputBuffer), "this String is from User Application; using METHOD_BUFFERED"))) {
        return;
    }
    printf("\nCalling DeviceIoControl METHOD_BUFFERED:\n");
    memset(OutputBuffer, 0, sizeof(OutputBuffer));
    bRc = DeviceIoControl(hDevice, (DWORD)IOCTL_NONPNP_METHOD_BUFFERED,
        InputBuffer, (DWORD)strlen(InputBuffer) + 1,
        OutputBuffer, sizeof(OutputBuffer),
        &bytesReturned, NULL);
    if (!bRc) {
        printf("Error in DeviceIoControl : %d", GetLastError());
        return;
    }
    printf("    OutBuffer (%d): %s\n", bytesReturned, OutputBuffer);


    // Performing METHOD_NEITHER
    printf("\nCalling DeviceIoControl METHOD_NEITHER\n");
    if (FAILED(StringCchCopy(InputBuffer, sizeof(InputBuffer), "this String is from User Application; using METHOD_NEITHER"))) {
        return;
    }
    memset(OutputBuffer, 0, sizeof(OutputBuffer));
    bRc = DeviceIoControl(hDevice, (DWORD)IOCTL_NONPNP_METHOD_NEITHER,
        InputBuffer, (DWORD)strlen(InputBuffer) + 1,
        OutputBuffer, sizeof(OutputBuffer),
        &bytesReturned, NULL);
    if (!bRc) {
        printf("Error in DeviceIoControl : %d\n", GetLastError());
        return;
    }
    printf("    OutBuffer (%d): %s\n", bytesReturned, OutputBuffer);

    // Performing METHOD_IN_DIRECT
    printf("\nCalling DeviceIoControl METHOD_IN_DIRECT\n");
    if (FAILED(StringCchCopy(InputBuffer, sizeof(InputBuffer), "this String is from User Application; using METHOD_IN_DIRECT"))) {
        return;
    }
    if (FAILED(StringCchCopy(OutputBuffer, sizeof(OutputBuffer), "This String is from User Application in OutBuffer; using METHOD_IN_DIRECT"))) {
        return;
    }
    bRc = DeviceIoControl(hDevice, (DWORD)IOCTL_NONPNP_METHOD_IN_DIRECT,
        InputBuffer, (DWORD)strlen(InputBuffer) + 1,
        OutputBuffer, sizeof(OutputBuffer),
        &bytesReturned, NULL);
    if (!bRc) {
        printf("Error in DeviceIoControl : : %d", GetLastError());
        return;
    }
    printf("    Number of bytes transfered from OutBuffer: %d\n", bytesReturned);

    // Performing METHOD_OUT_DIRECT
    printf("\nCalling DeviceIoControl METHOD_OUT_DIRECT\n");
    if (FAILED(StringCchCopy(InputBuffer, sizeof(InputBuffer), "this String is from User Application; using METHOD_OUT_DIRECT"))) {
        return;
    }
    memset(OutputBuffer, 0, sizeof(OutputBuffer));
    bRc = DeviceIoControl(hDevice, (DWORD)IOCTL_NONPNP_METHOD_OUT_DIRECT,
        InputBuffer, (DWORD)strlen(InputBuffer) + 1,
        OutputBuffer, sizeof(OutputBuffer),
        &bytesReturned, NULL);
    if (!bRc) {
        printf("Error in DeviceIoControl : : %d", GetLastError());
        return;
    }
    printf("    OutBuffer (%d): %s\n", bytesReturned, OutputBuffer);
}


#pragma warning(disable: 4100)

int main(int argc, const char* argv[])
{
    std::string driver_path;

    // Open the device
    ::HANDLE hDevice = ::CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE) {
        ::DWORD err = ::GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
            std::cout << "CreateFile(\"" << DEVICE_NAME << "\") error: " << ErrorMessage(err) << std::endl;
            return EXIT_FAILURE;
        }

        // The driver is not started yet so let us install the driver.
        // First setup full path to driver name.
        if (!InstallDriverFile(driver_path)) {
            return EXIT_FAILURE;
        }

        hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hDevice == INVALID_HANDLE_VALUE) {
            err = ::GetLastError();
            std::cout << "CreateFile(\"" << DEVICE_NAME << "\") error: " << ErrorMessage(err) << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Run the tests.
    DoIoctls(hDevice);

    // Close the handle to the device before unloading the driver.
    CloseHandle(hDevice);

    // Unload the driver, if was loaded.
    if (!driver_path.empty()) {
        RemoveDriverFile(driver_path);
    }
    return EXIT_SUCCESS;
}

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdarg>

#include <sys\cpusysregs.h>

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

#pragma warning(disable: 4100) // unreferenced formal parameter

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

bool GetDriverFile(std::string& driver_path)
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
    driver_path = file + '\\' + driver_basename;
    if (::GetFileAttributesA(driver_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Not found, replace '/exe/' with '/sys/' in the directory.
        pos = driver_path.rfind("\\exe\\");
        if (pos != std::string::npos) {
            driver_path.replace(pos + 1, 3, "sys");
        }
    }

    // Check if driver file exists.
    if (::GetFileAttributesA(driver_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cout << "Driver file not found: " << driver_path << std::endl;
        return false;
    }
    std::cout << "Driver: " << driver_path << std::endl;
    return true;
}

bool InitDriver(const std::string& DriverName, const std::string& DriverExe)
{
    // Connect to the Service Control Manager and open the Services database.
    const ::SC_HANDLE schSCManager = ::OpenSCManagerA(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == nullptr) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenSCManagerA() error: " << ErrorMessage(err) << std::endl;
        return false;
    }

    // Create a new service object.
    ::SC_HANDLE schService = ::CreateServiceA(schSCManager,           // handle of service control manager database
        DriverName.c_str(),             // address of name of service to start
        DriverName.c_str(),             // address of display name
        SERVICE_ALL_ACCESS,     // type of access to service
        SERVICE_KERNEL_DRIVER,  // type of service
        SERVICE_DEMAND_START,   // when to start service
        SERVICE_ERROR_NORMAL,   // severity if service fails to start
        DriverExe.c_str(),             // address of name of binary file
        NULL,                   // service does not belong to a group
        NULL,                   // no tag requested
        NULL,                   // no dependency names
        NULL,                   // use LocalSystem account
        NULL                    // no password for service account
    );
    bool ok = schService != nullptr;
    if (!ok) {
        ::DWORD err = ::GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            // Ignore this error.
            std::cout << "Service " << DriverName << " already exists" << std::endl;
            // Open existing service
            schService = ::OpenServiceA(schSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
            ok = schService != nullptr;
            err = ::GetLastError();
        }
        if (!ok) {
            std::cout << "CreateService()/OpenService() error: " << ErrorMessage(err) << std::endl;
            CloseServiceHandle(schSCManager);
            return false;
        }
    }

    // Start the execution of the service (i.e. start the driver).
    ok = ::StartServiceA(schService, 0, nullptr);
    if (!ok) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            // Ignore this error.
            std::cout << "Service " << DriverName << " already started" << std::endl;
            ok = true;
        }
        else {
            std::cout << "StartServiceA() error: " << ErrorMessage(err) << std::endl;
        }
    }

    // Close the service object.
    ::CloseServiceHandle(schService);

    // Close handle to service control manager.
    ::CloseServiceHandle(schSCManager);

    return ok;
}

bool TerminateDriver(const std::string& DriverName)
{
    // Connect to the Service Control Manager and open the Services database.
    const ::SC_HANDLE schSCManager = ::OpenSCManagerA(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == nullptr) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenSCManagerA() error: " << ErrorMessage(err) << std::endl;
        return false;
    }

    // Open the handle to the existing service.
    ::SC_HANDLE schService = ::OpenServiceA(schSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == nullptr) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenServiceA() error: " << ErrorMessage(err) << std::endl;
        ::CloseServiceHandle(schSCManager);
        return false;
    }

    // Request that the service stop.
    ::SERVICE_STATUS serviceStatus;
    bool ok = ::ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
    if (!ok) {
        const ::DWORD err = ::GetLastError();
        std::cout << "ControlService(STOP) error: " << ErrorMessage(err) << std::endl;
    }

    // Mark the service for deletion from the service control manager database.
    if (!::DeleteService(schService)) {
        const ::DWORD err = ::GetLastError();
        std::cout << "DeleteService() error: " << ErrorMessage(err) << std::endl;
        ok = false;
    }

    // Close the service object.
    ::CloseServiceHandle(schService);

    // Close handle to service control manager.
    ::CloseServiceHandle(schSCManager);

    return ok;
}


int main(int argc, const char* argv[])
{
    HANDLE hDevice;
    BOOL bRc;
    ULONG bytesReturned;
    DWORD errNum = 0;
    std::string driverLocation;
    char OutputBuffer[100];
    char InputBuffer[100];

    // open the device
    if ((hDevice = CreateFile( "\\\\.\\IoctlTest",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();

        if (errNum != ERROR_FILE_NOT_FOUND) {
            printf("CreateFile failed : %d\n", errNum);
            return EXIT_FAILURE;
        }

        // The driver is not started yet so let us the install the driver.
        if (!GetDriverFile(driverLocation) || !InitDriver(DRIVER_NAME, driverLocation)) {
            return EXIT_FAILURE;
        }

        hDevice = CreateFileA(CSR_DEVICE_NAME,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if ( hDevice == INVALID_HANDLE_VALUE ){
            printf ( "Error: CreatFile Failed : %d\n", GetLastError());
            return EXIT_FAILURE;
        }

    }

    // Printing Input & Output buffer pointers and size
    printf("InputBuffer Pointer = %p, BufLength = %Iu\n", InputBuffer, sizeof(InputBuffer));
    printf("OutputBuffer Pointer = %p BufLength = %Iu\n", OutputBuffer, sizeof(OutputBuffer));

    // Performing METHOD_BUFFERED
    StringCbCopy(InputBuffer, sizeof(InputBuffer), "This String is from User Application; using METHOD_BUFFERED");
    printf("\nCalling DeviceIoControl METHOD_BUFFERED:\n");
    memset(OutputBuffer, 0, sizeof(OutputBuffer));

    bRc = DeviceIoControl ( hDevice,
                            (DWORD) IOCTL_SIOCTL_METHOD_BUFFERED,
                            &InputBuffer,
                            (DWORD) strlen ( InputBuffer )+1,
                            &OutputBuffer,
                            sizeof( OutputBuffer),
                            &bytesReturned,
                            NULL
                            );

    if ( !bRc ) {
        printf ( "Error in DeviceIoControl : %d", GetLastError());
        return EXIT_FAILURE;
    }
    printf("Returned text: \"%.*s\"\n\n", int(bytesReturned), OutputBuffer);

    CloseHandle(hDevice);

    // Unload the driver.  Ignore any errors.
    TerminateDriver(DRIVER_NAME);
    return EXIT_SUCCESS;
}

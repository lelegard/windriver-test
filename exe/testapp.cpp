
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
#include <wdfinstaller.h>

#if defined(min)
#undef min
#endif

#include <string>
#include <iostream>
#include <algorithm>



bool InstallDriverFile(std::string& driver_path)
{
    // Driver file name.
    const std::string driver_file(DRIVER_NAME ".sys");

    // Get current executable.
    std::string exe(2048, ' ');
    size_t len = ::GetModuleFileNameA(nullptr, &exe[0], ::DWORD(exe.size()));
    exe.resize(std::min<size_t>(len, exe.size()));

    // Extract directory part.
    size_t pos = exe.rfind('\\');
    if (pos != std::string::npos) {
        exe.resize(pos);
    }
    std::cout << "App dir: " << exe << std::endl;

    // Check if driver is present in same directory as application.
    std::string infile(exe + '\\' + driver_file);
    if (::GetFileAttributesA(infile.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Not found, replace '/exe/' with '/sys' in the directory.
        pos = infile.rfind("\\exe\\");
        if (pos != std::string::npos) {
            infile.replace(pos + 1, 3, "sys");
        }
    }

    // Check if driver file exists.
    if (::GetFileAttributesA(infile.c_str()) == INVALID_FILE_ATTRIBUTES) {
        printf("Driver file %s not found\n", infile.c_str());
        return false;
    }
    std::cout << "Driver: " << infile << std::endl;

    // Get Windows directory (typically C:\Windows)
    driver_path.resize(MAX_PATH);
    len = ::GetWindowsDirectoryA(&driver_path[0], MAX_PATH);
    driver_path.resize(std::min<size_t>(len, MAX_PATH));

    // Build final driver location.
    driver_path += "\\System32\\Drivers\\";
    driver_path += driver_file;

    // Install the driver in system area.
    if (!::CopyFileA(infile.c_str(), driver_path.c_str(), false)) {
        printf("CopyFile() error: %d, destination: %s\n", ::GetLastError(), driver_path.c_str());
        return false;
    }

    // Connect to the Service Control Manager and open the Services database.
    const ::SC_HANDLE schSCManager = ::OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        printf("OpenSCManagerA() error: %d \n", ::GetLastError());
        return false;
    }

    // Create a new a service object.
    printf("CreateService(\"%s\", \"%s\")\n", DRIVER_NAME, driver_path.c_str());
    const ::SC_HANDLE schService = ::CreateServiceA(schSCManager,
        DRIVER_NAME,            // address of name of service to start
        DRIVER_NAME,            // address of display name
        SERVICE_ALL_ACCESS,     // type of access to service
        SERVICE_KERNEL_DRIVER,  // type of service
        SERVICE_DEMAND_START,   // when to start service
        SERVICE_ERROR_NORMAL,   // severity if service fails to start
        driver_path.c_str(),    // address of name of binary file
        NULL,                   // service does not belong to a group
        NULL,                   // no tag requested
        NULL,                   // no dependency names
        NULL,                   // use LocalSystem account
        NULL);                  // no password for service account

    bool ok = schService != NULL;
    if (!ok) {
        const ::DWORD err = ::GetLastError();
        if (err != ERROR_SERVICE_EXISTS) {
            printf("CreateService error: %d \n", err);
        }
    }
    else {
        // Start the execution of the service (i.e. start the driver).
        ok = ::StartServiceA(schService, 0, NULL);
        if (!ok) {
            const ::DWORD err = ::GetLastError();
            if (err != ERROR_SERVICE_ALREADY_RUNNING) {
                printf("StartServiceA error: %d \n", err);
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
        printf("OpenSCManagerA() error: %d \n", ::GetLastError());
        return false;
    }

    // Open the handle to the existing service.
    const ::SC_HANDLE schService = ::OpenServiceA(schSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
    bool ok = schService != NULL;
    if (!ok) {
        printf("OpenServiceA() error: %d \n", ::GetLastError());
    }
    else {
        // Request that the service stop.
        ::SERVICE_STATUS serviceStatus;
        ok = ::ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
        if (!ok) {
            printf("ControlService() error: %d \n", ::GetLastError());
        }
        else {
            // Mark the service for deletion from the service control manager database.
            ok = ::DeleteService(schService);
            if (!ok) {
                printf("DeleteService() error: %d \n", ::GetLastError());
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
    HANDLE   hDevice;
    DWORD    errNum = 0;
    std::string driver_path;

    // open the device
    hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();
        if (!(errNum == ERROR_FILE_NOT_FOUND || errNum == ERROR_PATH_NOT_FOUND)) {
            printf("CreateFile failed!  ERROR_FILE_NOT_FOUND = %d\n", errNum);
            return EXIT_FAILURE;
        }

        // The driver is not started yet so let us install the driver.
        // First setup full path to driver name.
        if (!InstallDriverFile(driver_path)) {
            return EXIT_FAILURE;
        }

        hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            printf ( "Error: CreatFile Failed : %d\n", GetLastError());
            return EXIT_FAILURE;
        }
    }

    DoIoctls(hDevice);

    // Close the handle to the device before unloading the driver.
    CloseHandle(hDevice);

    // Unload the driver. Ignore any errors.
    RemoveDriverFile(driver_path);
    return EXIT_SUCCESS;
}

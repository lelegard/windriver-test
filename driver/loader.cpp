// Driver loader / unloader application.
//
// There are two methods to load / unload the driver :
// 1) C++ application: this application
// 2) PowerShell: see loader.ps1

#include <windows.h>
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

// Build driver file name, in same directory as loader exe.
bool GetDriverFile(const std::string& driver_name, std::string& driver_path)
{
    // Get current executable.
    driver_path.resize(2048);
    size_t length = ::GetModuleFileNameA(nullptr, &driver_path[0], ::DWORD(driver_path.size()));
    driver_path.resize(std::min(length, driver_path.size()));

    // Extract directory part.
    size_t pos = driver_path.rfind('\\');
    if (pos != std::string::npos) {
        driver_path.resize(pos);
    }

    // Add driver name (same directory).
    driver_path += "\\" + driver_name + ".sys";

    // Check if driver file exists.
    if (::GetFileAttributesA(driver_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cout << "Driver file not found: " << driver_path << std::endl;
        return false;
    }

    std::cout << "Driver: " << driver_path << std::endl;
    return true;
}

// Load the driver.
bool LoadDriver(::SC_HANDLE sc_manager, const std::string& driver_name, const std::string& driver_file)
{
    // Create a new service object.
    ::SC_HANDLE service = ::CreateServiceA(
        sc_manager,             // Handle of service control manager database
        driver_name.c_str(),    // Name of service to start
        driver_name.c_str(),    // Display name
        SERVICE_ALL_ACCESS,     // Type of access to service
        SERVICE_KERNEL_DRIVER,  // Type of service
        SERVICE_DEMAND_START,   // When to start service
        SERVICE_ERROR_NORMAL,   // Severity if service fails to start
        driver_file.c_str(),    // Name of service driver binary file
        NULL,                   // Service does not belong to a group
        NULL,                   // No tag requested
        NULL,                   // No dependency names
        NULL,                   // Use LocalSystem account
        NULL);                  // No password for service account

    if (service == nullptr) {
        ::DWORD err = ::GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            // Ignore this error.
            std::cout << "Service " << driver_name << " already exists" << std::endl;
            // Open existing service
            service = ::OpenServiceA(sc_manager, driver_name.c_str(), SERVICE_ALL_ACCESS);
            err = ::GetLastError();
        }
        if (service == nullptr) {
            std::cout << "CreateService()/OpenService() error: " << ErrorMessage(err) << std::endl;
            return false;
        }
    }

    // Start the execution of the service (i.e. start the driver).
    bool ok = ::StartServiceA(service, 0, nullptr);
    if (!ok) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            // Ignore this error.
            std::cout << "Service " << driver_name << " already started" << std::endl;
            ok = true;
        }
        else {
            std::cout << "StartServiceA() error: " << ErrorMessage(err) << std::endl;
        }
    }

    // Close the service object.
    ::CloseServiceHandle(service);

    // Close handle to service control manager.
    ::CloseServiceHandle(sc_manager);

    return ok;
}

// Unload the driver.
bool UnloadDriver(::SC_HANDLE sc_manager, const std::string& driver_name)
{
    // Open the handle to the existing service.
    const ::SC_HANDLE service = ::OpenServiceA(sc_manager, driver_name.c_str(), SERVICE_ALL_ACCESS);
    if (service == nullptr) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenService() error: " << ErrorMessage(err) << std::endl;
        return false;
    }

    // Request a service stop.
    ::SERVICE_STATUS serviceStatus;
    bool ok = ::ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus);
    if (!ok) {
        const ::DWORD err = ::GetLastError();
        std::cout << "ControlService(STOP) error: " << ErrorMessage(err) << std::endl;
    }

    // Mark the service for deletion from the service control manager database.
    if (!::DeleteService(service)) {
        const ::DWORD err = ::GetLastError();
        std::cout << "DeleteService() error: " << ErrorMessage(err) << std::endl;
        ok = false;
    }

    // Close the service object.
    ::CloseServiceHandle(service);
    return ok;
}

// Loader application entry point.
int main(int argc, const char* argv[])
{
    // Load the driver by default. Unload with option "--unload".
    const bool unload = argc > 1 && std::string(argv[1]) == "--unload";

    // Connect to the Service Control Manager and open the Services database.
    const ::SC_HANDLE sc_manager = ::OpenSCManagerA(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (sc_manager == nullptr) {
        const ::DWORD err = ::GetLastError();
        std::cout << "OpenSCManager() error: " << ErrorMessage(err) << std::endl;
        return EXIT_FAILURE;
    }

    // Load or unload the driver
    std::string driver_file;
    const bool ok = unload ?
        UnloadDriver(sc_manager, CSR_DRIVER_NAME) :
        GetDriverFile(CSR_DRIVER_NAME, driver_file) && LoadDriver(sc_manager, CSR_DRIVER_NAME, driver_file);

    // Close handle to service control manager.
    ::CloseServiceHandle(sc_manager);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

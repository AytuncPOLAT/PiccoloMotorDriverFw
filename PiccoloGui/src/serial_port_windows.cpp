#include "serial_port_windows.hpp"

#define NOMINMAX
#include <Windows.h>
#include <algorithm>

SerialPortWindows::SerialPortWindows()
    : handle_(reinterpret_cast<void*>(INVALID_HANDLE_VALUE))
{
}

SerialPortWindows::~SerialPortWindows()
{
    disconnect();
}

std::vector<std::string> SerialPortWindows::listAvailablePorts(int maxPort) const
{
    std::vector<std::string> ports;
    maxPort = std::max(1, maxPort);
    for (int i = 1; i <= maxPort; ++i)
    {
        const std::string name = "COM" + std::to_string(i);
        const std::string fullName = "\\\\.\\" + name;

        HANDLE portHandle = CreateFileA(fullName.c_str(),
                                        GENERIC_READ | GENERIC_WRITE,
                                        0,
                                        nullptr,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        nullptr);

        if (portHandle != INVALID_HANDLE_VALUE)
        {
            ports.push_back(name);
            CloseHandle(portHandle);
        }
    }
    return ports;
}

bool SerialPortWindows::connect(const std::string& portName, int baudRate, std::string& errorMessage)
{
    disconnect();

    const std::string fullName = "\\\\.\\" + portName;
    HANDLE serialHandle = CreateFileA(fullName.c_str(),
                                      GENERIC_READ | GENERIC_WRITE,
                                      0,
                                      nullptr,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      nullptr);

    if (serialHandle == INVALID_HANDLE_VALUE)
    {
        errorMessage = "Unable to open " + portName;
        return false;
    }

    DCB serialParams = {};
    serialParams.DCBlength = sizeof(serialParams);
    if (!GetCommState(serialHandle, &serialParams))
    {
        CloseHandle(serialHandle);
        errorMessage = "GetCommState failed for " + portName;
        return false;
    }

    serialParams.BaudRate = static_cast<DWORD>(baudRate);
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;

    if (!SetCommState(serialHandle, &serialParams))
    {
        CloseHandle(serialHandle);
        errorMessage = "SetCommState failed for " + portName;
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(serialHandle, &timeouts))
    {
        CloseHandle(serialHandle);
        errorMessage = "SetCommTimeouts failed for " + portName;
        return false;
    }

    PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    handle_ = reinterpret_cast<void*>(serialHandle);
    portName_ = portName;
    return true;
}

void SerialPortWindows::disconnect()
{
    auto serialHandle = reinterpret_cast<HANDLE>(handle_);
    if (serialHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(serialHandle);
        handle_ = reinterpret_cast<void*>(INVALID_HANDLE_VALUE);
    }
    portName_.clear();
}

bool SerialPortWindows::isConnected() const
{
    return reinterpret_cast<HANDLE>(handle_) != INVALID_HANDLE_VALUE;
}

std::string SerialPortWindows::connectedPort() const
{
    return portName_;
}

bool SerialPortWindows::write(const uint8_t* data, std::size_t size, std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    auto serialHandle = reinterpret_cast<HANDLE>(handle_);
    DWORD bytesWritten = 0;
    const BOOL ok = WriteFile(serialHandle, data, static_cast<DWORD>(size), &bytesWritten, nullptr);

    if (!ok || bytesWritten != size)
    {
        errorMessage = "Write failed.";
        return false;
    }

    return true;
}

bool SerialPortWindows::read(uint8_t* buffer, std::size_t maxSize, std::size_t& bytesRead, std::string& errorMessage)
{
    bytesRead = 0;

    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    auto serialHandle = reinterpret_cast<HANDLE>(handle_);
    COMSTAT status = {};
    DWORD errors = 0;
    if (!ClearCommError(serialHandle, &errors, &status))
    {
        errorMessage = "ClearCommError failed.";
        return false;
    }

    if (status.cbInQue == 0)
    {
        return true;
    }

    const DWORD toRead = std::min(static_cast<DWORD>(maxSize), status.cbInQue);
    DWORD dwRead = 0;
    if (!ReadFile(serialHandle, buffer, toRead, &dwRead, nullptr))
    {
        errorMessage = "Read failed.";
        return false;
    }

    bytesRead = dwRead;
    return true;
}

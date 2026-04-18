#include "serial_port.hpp"
#include "serial_port_windows.hpp"
#include "serial_port_unix.hpp"

std::unique_ptr<ISerialPort> CreateSerialPort()
{
#ifdef _WIN32
    return std::make_unique<SerialPortWindows>();
#else
    return std::make_unique<SerialPortUnix>();
#endif
}

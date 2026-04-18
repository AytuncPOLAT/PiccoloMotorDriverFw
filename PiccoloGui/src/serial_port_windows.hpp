#pragma once

#include "serial_port.hpp"

class SerialPortWindows : public ISerialPort
{
public:
    SerialPortWindows();
    ~SerialPortWindows() override;

    std::vector<std::string> listAvailablePorts(int maxPort = 30) const override;
    bool connect(const std::string& portName, int baudRate, std::string& errorMessage) override;
    void disconnect() override;
    bool isConnected() const override;
    std::string connectedPort() const override;

    bool write(const uint8_t* data, std::size_t size, std::string& errorMessage) override;
    bool read(uint8_t* buffer, std::size_t maxSize, std::size_t& bytesRead, std::string& errorMessage) override;

private:
    void* handle_;
    std::string portName_;
};

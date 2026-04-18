#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

class ISerialPort
{
public:
    virtual ~ISerialPort() = default;

    virtual std::vector<std::string> listAvailablePorts(int maxPort = 30) const = 0;
    virtual bool connect(const std::string& portName, int baudRate, std::string& errorMessage) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual std::string connectedPort() const = 0;

    virtual bool write(const uint8_t* data, std::size_t size, std::string& errorMessage) = 0;
    virtual bool read(uint8_t* buffer, std::size_t maxSize, std::size_t& bytesRead, std::string& errorMessage) = 0;
};

std::unique_ptr<ISerialPort> CreateSerialPort();

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(x)
#endif
#include "../../AppLayer/Common/SystemData.hpp"
#if defined(_MSC_VER) && !defined(__clang__)
#undef __attribute__
#endif

class ISerialPort;

class SerialManager
{
public:
    SerialManager();
    ~SerialManager();

    std::vector<std::string> listAvailablePorts(int maxPort = 30) const;

    bool connect(const std::string& portName, int baudRate, std::string& errorMessage);
    void disconnect();

    bool isConnected() const;
    std::string connectedPort() const;

    bool writeLine(const std::string& line, std::string& errorMessage);
    std::vector<std::string> takeDebugMessages();
    std::vector<std::string> pollLines(std::string& errorMessage);
    bool sendPing(uint8_t deviceAddress, std::string& errorMessage, int timeoutMs = 2000);
    bool readProperty(uint8_t deviceAddress,
                      Common::PROPERTY property,
                      int32_t& valueOut,
                      std::string& errorMessage,
                      int timeoutMs = 100);
    bool readProperty(uint8_t deviceAddress,
                      Common::PROPERTY property,
                      int32_t& value0Out,
                      int32_t& value1Out,
                      int32_t& value2Out,
                      int32_t& value3Out,
                      std::string& errorMessage,
                      int timeoutMs = 100);
    bool writeProperty(uint8_t deviceAddress, Common::PROPERTY property, int32_t value, std::string& errorMessage);
    bool sendFlashWriteCommand(uint8_t deviceAddress, std::string& errorMessage);
    bool sendArmCommand(uint8_t deviceAddress, std::string& errorMessage);
    bool sendDisarmCommand(uint8_t deviceAddress, std::string& errorMessage);

private:
    std::unique_ptr<ISerialPort> serialPort_;
    std::string pendingReadBuffer_;
    std::vector<std::string> debugMessages_;
};

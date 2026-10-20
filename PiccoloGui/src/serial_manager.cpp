#include "serial_manager.hpp"
#include "serial_port.hpp"
#include "SystemData.hpp"
#include "../../AppLayer/Common/Crc16.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <stdio.h>
#include <iostream>

#define RESPONSE_TIMEOUT_MS 500

namespace
{
static_assert(sizeof(Common::CANBusFrame) == 9, "CANBusFrame size must match firmware protocol (9 bytes)");
static_assert(sizeof(Common::SerialFrame) == 11, "SerialFrame size must match firmware protocol (11 bytes)");

std::string formatPingFrame(const Common::SerialFrame& frame, uint16_t expectedCrc)
{
    char frameText[256] = {};
    std::snprintf(frameText,
                  sizeof(frameText),
                  "Ping response frame: cmd=%u address=%u subAddress=%u checksum=0x%04X expected=0x%04X",
                  static_cast<unsigned int>(static_cast<uint8_t>(frame.canFrame.command)),
                  static_cast<unsigned int>(frame.canFrame.messageID),
                  static_cast<unsigned int>(frame.canFrame.registerAddress),
                  static_cast<unsigned int>(frame.checksum),
                  static_cast<unsigned int>(expectedCrc));
    return frameText;
}

void populateSerialFrame(Common::SerialFrame& frame,
                         uint8_t deviceAddress,
                         Common::CMD_TYPE cmd,
                         uint8_t subAddress,
                         const uint8_t* payload,
                         uint8_t flags = 0,
                         std::size_t payloadLen = 4)
{
    frame.canFrame.messageID = deviceAddress;
    frame.canFrame.registerAddress = subAddress;
    frame.canFrame.command = cmd;
    frame.canFrame.flags = flags;
    frame.canFrame.sourceID = 1;
    std::memset(frame.canFrame.data, 0, sizeof(frame.canFrame.data));
    if (payload != nullptr && payloadLen > 0)
    {
        std::memcpy(frame.canFrame.data, payload, std::min<std::size_t>(sizeof(frame.canFrame.data), payloadLen));
    }
}
} // namespace

SerialManager::SerialManager()
    : serialPort_(CreateSerialPort())
{
}

SerialManager::~SerialManager()
{
    disconnect();
}

std::vector<std::string> SerialManager::listAvailablePorts(int maxPort) const
{
    return serialPort_->listAvailablePorts(maxPort);
}

bool SerialManager::connect(const std::string& portName, int baudRate, std::string& errorMessage)
{
    disconnect();
    return serialPort_->connect(portName, baudRate, errorMessage);
}

void SerialManager::disconnect()
{
    serialPort_->disconnect();
    pendingReadBuffer_.clear();
}

bool SerialManager::isConnected() const
{
    return serialPort_->isConnected();
}

std::string SerialManager::connectedPort() const
{
    return serialPort_->connectedPort();
}

bool SerialManager::writeLine(const std::string& line, std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    const std::string payload = line + "\r\n";
    return serialPort_->write(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), errorMessage);
}

std::vector<std::string> SerialManager::takeDebugMessages()
{
    auto messages = std::move(debugMessages_);
    debugMessages_.clear();
    return messages;
}

std::vector<std::string> SerialManager::pollLines(std::string& errorMessage)
{
    std::vector<std::string> lines;

    if (!isConnected())
    {
        return lines;
    }

    uint8_t readBuffer[512] = {};
    std::size_t bytesRead = 0;
    while (serialPort_->read(readBuffer, sizeof(readBuffer), bytesRead, errorMessage))
    {
        if (bytesRead > 0)
        {
            pendingReadBuffer_.append(reinterpret_cast<const char*>(readBuffer), bytesRead);
        }
        else
        {
            break;
        }
    }

    if (!errorMessage.empty())
    {
        return lines;
    }

    std::size_t newlinePos = std::string::npos;
    while ((newlinePos = pendingReadBuffer_.find('\n')) != std::string::npos)
    {
        std::string line = pendingReadBuffer_.substr(0, newlinePos);
        pendingReadBuffer_.erase(0, newlinePos + 1);

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        lines.push_back(line);
    }

    return lines;
}

bool SerialManager::sendPing(uint8_t deviceAddress, std::string& errorMessage, int timeoutMs)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::PING, 0, nullptr);
    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));
    debugMessages_.push_back("Ping TX: cmd=0 address=" + std::to_string(deviceAddress));

    if (!serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage))
    {
        debugMessages_.push_back("Ping debug: write failed.");
        return false;
    }

    debugMessages_.push_back("Ping debug: waiting for response...");

    const auto start = std::chrono::steady_clock::now();
    uint8_t rxBuffer[sizeof(Common::SerialFrame)] = {};

    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
           < RESPONSE_TIMEOUT_MS)
    {
        std::string readError;
        std::size_t bytesRead = 0;
        if (!serialPort_->read(rxBuffer, sizeof(rxBuffer), bytesRead, readError))
        {
            if (!readError.empty())
            {
                errorMessage = "Ping read failed: " + readError;
                return false;
            }
        }

        std::cout << "Ping debug: bytesRead=" << bytesRead << std::endl;

        if (bytesRead == sizeof(Common::SerialFrame))
        {
            Common::SerialFrame rx = {};
            std::memcpy(&rx, rxBuffer, sizeof(rx));

            const uint16_t expectedCrc
                = crc.Calculate(0, reinterpret_cast<uint8_t*>(&rx), sizeof(rx) - sizeof(rx.checksum));

            if (rx.checksum == expectedCrc && rx.canFrame.command == Common::CMD_TYPE::PING_RESPONSE
                && rx.canFrame.messageID == deviceAddress)
            {
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
                debugMessages_.push_back("Ping O.K. (latency: " + std::to_string(latency) + " ms)");
                return true;
            }

            else if (rx.checksum != expectedCrc)
            {
                errorMessage = "Property read response: checksum mismatch.";
                return false;
            }

            else if (rx.canFrame.command != Common::CMD_TYPE::PING_RESPONSE)
            {
                errorMessage = "Property read response: unexpected command.";
                return false;
            }

            else if (rx.canFrame.messageID != deviceAddress)
            {
                errorMessage = "Property read response: unexpected device address.";
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    errorMessage = "Ping timeout.";
    debugMessages_.push_back("Ping debug: timeout waiting for response.");
    return false;
}

/////////////////////////////////////////////////////////////////////
// Read Property
/////////////////////////////////////////////////////////////////////

bool SerialManager::readProperty(uint8_t deviceAddress,
                                 Common::PROPERTY property,
                                 int32_t& valueOut,
                                 std::string& errorMessage,
                                 int timeoutMs)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    uint8_t payload[4] = {};
    std::memcpy(payload, &property, sizeof(property));
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::READ_FROM_DEVICE, static_cast<uint8_t>(property), payload);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    if (!serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    uint8_t rxBuffer[128] = {};
    std::size_t rxCount = 0;

    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
           < RESPONSE_TIMEOUT_MS)
    {
        std::string readError;
        std::size_t bytesRead = 0;
        if (!serialPort_->read(rxBuffer + rxCount, sizeof(rxBuffer) - rxCount, bytesRead, readError))
        {
            if (!readError.empty())
            {
                errorMessage = "Property read response read failed: " + readError;
                return false;
            }
        }

        if (bytesRead > 0)
        {
            rxCount += bytesRead;
        }

        if (rxCount >= sizeof(Common::SerialFrame))
        {
            for (std::size_t offset = 0; offset <= rxCount - sizeof(Common::SerialFrame); ++offset)
            {
                Common::SerialFrame rx = {};
                std::memcpy(&rx, rxBuffer + offset, sizeof(rx));

                const uint16_t expectedCrc
                    = crc.Calculate(0, reinterpret_cast<uint8_t*>(&rx), sizeof(rx) - sizeof(rx.checksum));

                if (rx.checksum == expectedCrc && rx.canFrame.command == Common::CMD_TYPE::READ_FROM_DEVICE
                    && rx.canFrame.messageID == deviceAddress && rx.canFrame.registerAddress == static_cast<uint8_t>(property))
                {
                    std::memcpy(&valueOut, rx.canFrame.data, sizeof(valueOut));
                    return true;
                }
            }

            if (rxCount >= sizeof(rxBuffer) - 1)
            {
                errorMessage = "Property read response: no valid frame found in buffer.";
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    errorMessage = "Property read timeout.";
    return false;
}

bool SerialManager::readProperty(uint8_t deviceAddress,
                                 Common::PROPERTY property,
                                 int32_t& value0Out,
                                 int32_t& value1Out,
                                 int32_t& value2Out,
                                 int32_t& value3Out,
                                 std::string& errorMessage,
                                 int timeoutMs)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    uint8_t payload[4] = {};
    std::memcpy(payload, &property, sizeof(property));
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::READ_FROM_DEVICE, static_cast<uint8_t>(property), payload);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    if (!serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    const auto start = std::chrono::steady_clock::now();
    uint8_t rxBuffer[256] = {};
    std::size_t rxCount = 0;
    bool gotPos = false;
    bool gotSpeed = false;
    bool gotTorque = false;
    bool gotCurrent = false;

    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
           < RESPONSE_TIMEOUT_MS)
    {
        std::string readError;
        std::size_t bytesRead = 0;
        if (!serialPort_->read(rxBuffer + rxCount, sizeof(rxBuffer) - rxCount, bytesRead, readError))
        {
            if (!readError.empty())
            {
                errorMessage = "Property read response read failed: " + readError;
                return false;
            }
        }

        if (bytesRead > 0)
        {
            rxCount += bytesRead;
        }

        if (rxCount >= sizeof(Common::SerialFrame))
        {
            for (std::size_t offset = 0; offset <= rxCount - sizeof(Common::SerialFrame); ++offset)
            {
                Common::SerialFrame rx = {};
                std::memcpy(&rx, rxBuffer + offset, sizeof(rx));

                const uint16_t expectedCrc
                    = crc.Calculate(0, reinterpret_cast<uint8_t*>(&rx), sizeof(rx) - sizeof(rx.checksum));

                if (rx.checksum == expectedCrc && rx.canFrame.command == Common::CMD_TYPE::READ_FROM_DEVICE
                    && rx.canFrame.messageID == deviceAddress)
                {
                    if (rx.canFrame.registerAddress == static_cast<uint8_t>(Common::PROPERTY::MULTI_TURN_ENCODER))
                    {
                        std::memcpy(&value0Out, rx.canFrame.data, sizeof(value0Out));
                        gotPos = true;
                    }
                    else if (rx.canFrame.registerAddress == static_cast<uint8_t>(Common::PROPERTY::SPEED_GET))
                    {
                        std::memcpy(&value1Out, rx.canFrame.data, sizeof(value1Out));
                        gotSpeed = true;
                    }
                    else if (rx.canFrame.registerAddress == static_cast<uint8_t>(Common::PROPERTY::TORQUE_GET))
                    {
                        std::memcpy(&value2Out, rx.canFrame.data, sizeof(value2Out));
                        gotTorque = true;
                    }
                    else if (rx.canFrame.registerAddress == static_cast<uint8_t>(Common::PROPERTY::MOTOR_CURRENT))
                    {
                        std::memcpy(&value3Out, rx.canFrame.data, sizeof(value3Out));
                        gotCurrent = true;
                    }
                    else if (rx.canFrame.registerAddress == static_cast<uint8_t>(property))
                    {
                        std::memcpy(&value0Out, rx.canFrame.data, sizeof(value0Out));
                        gotPos = true;
                    }
                }
            }

            if (gotPos && gotSpeed && gotTorque)
            {
                return true;
            }

            if (rxCount >= sizeof(rxBuffer) - 1)
            {
                errorMessage = "Property read response: buffer full without all telemetry frames.";
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (gotPos)
    {
        return true;
    }

    errorMessage = "Property read timeout.";
    return false;
}

bool SerialManager::writeProperty(uint8_t deviceAddress,
                                  Common::PROPERTY property,
                                  int32_t value,
                                  std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    uint8_t payload[4] = {};
    std::memcpy(payload, &value, sizeof(value));
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::WRITE_TO_DEVICE, static_cast<uint8_t>(property), payload);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    return serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage);
}

bool SerialManager::writeProperty(Common::SerialFrame packet,
                                  std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::Crc16 crc;
    packet.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&packet), sizeof(packet) - sizeof(packet.checksum));

    return serialPort_->write(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet), errorMessage);
}

bool SerialManager::sendFlashWriteCommand(uint8_t deviceAddress, std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::WRITE_TO_DEVICE_FLASH, 0, nullptr);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    return serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage);
}

bool SerialManager::sendArmCommand(uint8_t deviceAddress, std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::DRIVER_ARM, 0, nullptr);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    return serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage);
}

bool SerialManager::sendDisarmCommand(uint8_t deviceAddress, std::string& errorMessage)
{
    if (!isConnected())
    {
        errorMessage = "No serial connection.";
        return false;
    }

    Common::SerialFrame tx = {};
    populateSerialFrame(tx, deviceAddress, Common::CMD_TYPE::DRIVER_DISARM, 0, nullptr);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    return serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage);
}

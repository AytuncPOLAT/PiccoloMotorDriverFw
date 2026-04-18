#include "serial_manager.hpp"
#include "serial_port.hpp"
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

namespace
{
constexpr uint8_t kCmdPing = 0;
constexpr uint8_t kCmdPingResponse = 1;
constexpr uint8_t kCmdReadFromDevice = 2;
constexpr uint8_t kCmdWriteToDevice = 3;

#pragma pack(push, 1)
struct DataFrame
{
    uint8_t cmd;
    uint8_t address;
    uint32_t data0;
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
    uint16_t checksum;
};
#pragma pack(pop)

static_assert(sizeof(DataFrame) == 20, "DataFrame size must match firmware protocol");

std::string formatPingFrame(const DataFrame& frame, uint16_t expectedCrc)
{
    char frameText[256] = {};
    std::snprintf(frameText,
                  sizeof(frameText),
                  "Ping response frame: cmd=%u address=%u data0=%lu data1=%lu data2=%lu data3=%lu checksum=0x%04X expected=0x%04X",
                  static_cast<unsigned int>(frame.cmd),
                  static_cast<unsigned int>(frame.address),
                  static_cast<unsigned long>(frame.data0),
                  static_cast<unsigned long>(frame.data1),
                  static_cast<unsigned long>(frame.data2),
                  static_cast<unsigned long>(frame.data3),
                  static_cast<unsigned int>(frame.checksum),
                  static_cast<unsigned int>(expectedCrc));
    return frameText;
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

    DataFrame tx = {};
    tx.cmd = kCmdPing;
    tx.address = deviceAddress;
    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));
    debugMessages_.push_back("Ping TX: cmd=0 address=" + std::to_string(deviceAddress));

    if (!serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage))
    {
        debugMessages_.push_back("Ping debug: write failed.");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    debugMessages_.push_back("Ping debug: waiting for response...");

    const auto start = std::chrono::steady_clock::now();

    DataFrame rxBuffer = {};

    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
           < timeoutMs)
    {
        std::string readError;
        std::size_t bytesRead = 0;
        if (!serialPort_->read((uint8_t*)&rxBuffer, sizeof(rxBuffer), bytesRead, readError))
        {
            if (!readError.empty())
            {
                errorMessage = "Ping read failed: " + readError;
                return false;
            }
        }

        if (bytesRead >= sizeof(DataFrame))
        {
            DataFrame rx = {};
            std::memcpy(&rx, &rxBuffer, sizeof(rx));

            const uint16_t expectedCrc
                = crc.Calculate(0, reinterpret_cast<uint8_t*>(&rx), sizeof(rx) - sizeof(rx.checksum));
            debugMessages_.push_back(formatPingFrame(rx, expectedCrc));

            if (rx.checksum != expectedCrc)
            {
                errorMessage = "Ping response CRC mismatch.";
                debugMessages_.push_back("Ping debug: CRC mismatch.");
                return false;
            }
            if (rx.cmd != kCmdPingResponse)
            {
                errorMessage = "Unexpected response command.";
                debugMessages_.push_back("Ping debug: unexpected response command.");
                return false;
            }
            if (rx.address != deviceAddress)
            {
                errorMessage = "Ping response address mismatch.";
                debugMessages_.push_back("Ping debug: address mismatch.");
                return false;
            }

            debugMessages_.push_back("Ping debug: valid response received.");
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    errorMessage = "Ping timeout.";
    debugMessages_.push_back("Ping debug: timeout waiting for response.");
    return false;
}

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

    DataFrame tx = {};
    tx.cmd = kCmdReadFromDevice;
    tx.address = deviceAddress;
    tx.data0 = static_cast<uint32_t>(property);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    if (!serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage))
    {
        return false;
    }

    const auto start = std::chrono::steady_clock::now();
    uint8_t rxBuffer[sizeof(DataFrame)] = {};
    std::size_t rxCount = 0;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
           < timeoutMs)
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

        if (rxCount >= sizeof(DataFrame))
        {
            DataFrame rx = {};
            std::memcpy(&rx, rxBuffer, sizeof(rx));

            const uint16_t expectedCrc
                = crc.Calculate(0, reinterpret_cast<uint8_t*>(&rx), sizeof(rx) - sizeof(rx.checksum));

            if (rx.checksum != expectedCrc)
            {
                errorMessage = "Property read response CRC mismatch.";
                std::cout << "Property read debug: CRC mismatch. Received checksum=0x" << std::hex << rx.checksum
                     << " expected=0x" << expectedCrc << std::dec << std::endl; 
                //return false;
            }
            if (rx.cmd != kCmdReadFromDevice)
            {
                errorMessage = "Unexpected property read response command.";
                return false;
            }
            if (rx.address != deviceAddress)
            {
                errorMessage = "Property read response address mismatch.";
                return false;
            }

            valueOut = static_cast<int32_t>(rx.data0);
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    DataFrame tx = {};
    tx.cmd = kCmdWriteToDevice;
    tx.address = deviceAddress;
    tx.data0 = static_cast<uint32_t>(property);
    tx.data1 = static_cast<uint32_t>(value);

    Common::Crc16 crc;
    tx.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&tx), sizeof(tx) - sizeof(tx.checksum));

    return serialPort_->write(reinterpret_cast<const uint8_t*>(&tx), sizeof(tx), errorMessage);
}

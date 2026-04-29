#include "log_utils.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sstream>

std::string timestampNow()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t nowTime = system_clock::to_time_t(now);

    std::tm tmNow = {};
#ifdef _WIN32
    localtime_s(&tmNow, &nowTime);
#else
    localtime_r(&nowTime, &tmNow);
#endif

    char buf[16] = {};
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    return std::string(buf);
}

bool loggingEnabled = true;

void addLog(std::vector<std::string>& logs, const std::string& line)
{
    if (!loggingEnabled)
    {
        return;
    }

    logs.push_back("[" + timestampNow() + "] " + line);

    constexpr std::size_t kMaxLogLines = 2000;
    if (logs.size() > kMaxLogLines)
    {
        logs.erase(logs.begin(), logs.begin() + static_cast<long long>(logs.size() - kMaxLogLines));
    }
}

bool parseTelemetryCsv(const std::string& line, double& encoder, double& speedRps, double& torque, double& busVoltage, double& pwmPercent, double& driverTemp, double& motorTemp)
{
    // Expected format: encoder,speed,torque,busVoltage,pwmPercent,driverTemp,motorTemp
    std::stringstream ss(line);
    std::string token;
    std::array<double, 7> values = {};

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (!std::getline(ss, token, ','))
        {
            return false;
        }

        char* endPtr = nullptr;
        const double parsed = std::strtod(token.c_str(), &endPtr);
        if (endPtr == token.c_str())
        {
            return false;
        }
        values[i] = parsed;
    }

    encoder = values[0];
    speedRps = values[1];
    torque = values[2];
    busVoltage = values[3];
    pwmPercent = values[4];
    driverTemp = values[5];
    motorTemp = values[6];
    return true;
}

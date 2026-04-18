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

void addLog(std::vector<std::string>& logs, const std::string& line)
{
    logs.push_back("[" + timestampNow() + "] " + line);

    constexpr std::size_t kMaxLogLines = 2000;
    if (logs.size() > kMaxLogLines)
    {
        logs.erase(logs.begin(), logs.begin() + static_cast<long long>(logs.size() - kMaxLogLines));
    }
}

bool parseTelemetryCsv(const std::string& line, double& speedRps, double& currentA, double& positionDeg)
{
    // Expected format: speed,current,position
    std::stringstream ss(line);
    std::string token;
    std::array<double, 3> values = {};

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

    speedRps = values[0];
    currentA = values[1];
    positionDeg = values[2];
    return true;
}

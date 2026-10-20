#pragma once

#if defined(__MINGW32__)
#include <ctime>

inline int piccoloGmtimeS(std::tm* result, const std::time_t* time)
{
    const std::tm* value = std::gmtime(time);
    if (value == nullptr)
    {
        return 1;
    }
    *result = *value;
    return 0;
}

inline int piccoloLocaltimeS(std::tm* result, const std::time_t* time)
{
    const std::tm* value = std::localtime(time);
    if (value == nullptr)
    {
        return 1;
    }
    *result = *value;
    return 0;
}

inline std::time_t piccoloMkgmtime(std::tm* value)
{
    return std::mktime(value);
}

#define gmtime_s piccoloGmtimeS
#define localtime_s piccoloLocaltimeS
#define _mkgmtime piccoloMkgmtime
#endif

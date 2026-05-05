#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(x)
#endif
#include "../../AppLayer/Common/SystemData.hpp"
#if defined(_MSC_VER) && !defined(__clang__)
#undef __attribute__
#endif

enum class ConfigCategory : uint8_t
{
    DEVICE = 0,
    PID,
    MOTOR_PARAMETERS
};

inline const char* toCategoryLabel(ConfigCategory category)
{
    switch (category)
    {
        case ConfigCategory::DEVICE:
            return "Device";
        case ConfigCategory::PID:
            return "PID Controllers";
        case ConfigCategory::MOTOR_PARAMETERS:
            return "Motor Parameters";
        default:
            return "Unknown";
    }
}

struct ConfigItem
{
    Common::PROPERTY property;
    ConfigCategory category;
    const char* label;
    int32_t value;
};

inline const char* toPropertyLabel(Common::PROPERTY property)
{
    using Property = Common::PROPERTY;
    switch (property)
    {
        case Property::FLASH_MAGIC:
            return "Flash Magic";
        case Property::SERIAL_NO:
            return "Serial Number";
        case Property::FW_VERSION:
            return "Firmware Version";
        case Property::DEV_ADDRESS:
            return "Device Address";
        case Property::DEV_CONTROL_MODE:
            return "Control Mode";
        case Property::PID_DQ_KP:
            return "PID DQ Kp";
        case Property::PID_DQ_KI:
            return "PID DQ Ki";
        case Property::PID_DQ_KD:
            return "PID DQ Kd";
        case Property::PID_DQ_MAX_INTEGRAL_WU:
            return "PID DQ Max Integral WU";
        case Property::PID_DQ_SAT:
            return "PID DQ Saturation";
        case Property::PID_SPD_KP:
            return "PID Speed Kp";
        case Property::PID_SPD_KI:
            return "PID Speed Ki";
        case Property::PID_SPD_KD:
            return "PID Speed Kd";
        case Property::PID_SPD_MAX_INTEGRAL_WU:
            return "PID Speed Max Integral WU";
        case Property::PID_SPD_SAT:
            return "PID Speed Saturation";
        case Property::PID_POS_KP:
            return "PID Position Kp";
        case Property::PID_POS_KI:
            return "PID Position Ki";
        case Property::PID_POS_KD:
            return "PID Position Kd";
        case Property::PID_POS_MAX_INTEGRAL_WU:
            return "PID Position Max Integral WU";
        case Property::PID_POS_SAT:
            return "PID Position Saturation";
        case Property::MOTOR_ENCODER_OFFSET:
            return "Motor Encoder Offset";
        case Property::MOTOR_POLES:
            return "Motor Poles";
        case Property::DC_BUS_VOLTAGE:
            return "DC Bus Voltage";
        case Property::MULTI_TURN_ENCODER:
            return "Multi Turn Encoder";
        case Property::CURRENT_AMPLIFIER_GAIN:
            return "Current Amplifier Gain";
        case Property::POSITION_HOME_MIN:
            return "Position Home Min";
        case Property::POSITION_HOME_MAX:
            return "Position Home Max";
        default:
            return "Unknown";
    }
}

inline int32_t toDefaultPropertyValue(Common::PROPERTY property)
{
    using Property = Common::PROPERTY;
    switch (property)
    {
        case Property::FLASH_MAGIC:
            return static_cast<int32_t>(Common::FLASH_MAGIC_NUM);
        case Property::DEV_ADDRESS:
            return 1;
        case Property::MOTOR_POLES:
            return static_cast<int32_t>(Common::MOTOR_POLES);
        case Property::POSITION_HOME_MIN:
            return -1000000;
        case Property::POSITION_HOME_MAX:
            return 1000000;
        default:
            return 0;
    }
}

inline ConfigCategory toPropertyCategory(Common::PROPERTY property)
{
    using Property = Common::PROPERTY;
    const int propValue = static_cast<int>(property);

    // First 5 items (0-4): Device
    if (propValue <= 4)
    {
        return ConfigCategory::DEVICE;
    }
    // Items 5-19: PID Controllers
    else if (propValue <= 19)
    {
        return ConfigCategory::PID;
    }
    // Items 20+: Motor Parameters
    else
    {
        return ConfigCategory::MOTOR_PARAMETERS;
    }
}

inline std::vector<ConfigItem> buildConfigItemsFromPropertyEnum()
{
    using Property = Common::PROPERTY;
    const int first = static_cast<int>(Property::FLASH_MAGIC);
    const int last = static_cast<int>(Property::POSITION_HOME_MAX);

    std::vector<ConfigItem> items;
    items.reserve(static_cast<std::size_t>(last - first + 1));
    for (int value = first; value <= last; ++value)
    {
        const auto property = static_cast<Property>(value);
        items.push_back(
            {property, toPropertyCategory(property), toPropertyLabel(property), toDefaultPropertyValue(property)});
    }

    return items;
};

struct TelemetryBuffer
{
    std::deque<double> t;
    std::deque<double> speed;
    std::deque<double> current;
    std::deque<double> position;
    std::deque<double> busVoltage;
    std::deque<double> pwmPercent;
    std::deque<double> driverTemp;
    std::deque<double> motorTemp;
    std::deque<double> multiTurnEncoder;
    std::deque<double> torque;
    std::size_t capacity = 1200;
    double timeWindowSeconds = 10.0;

    void trimToWindow()
    {
        while (!t.empty() && (t.back() - t.front()) > timeWindowSeconds)
        {
            t.pop_front();
            speed.pop_front();
            current.pop_front();
            position.pop_front();
            busVoltage.pop_front();
            pwmPercent.pop_front();
            driverTemp.pop_front();
            motorTemp.pop_front();
            multiTurnEncoder.pop_front();
            torque.pop_front();
        }

        while (t.size() > capacity)
        {
            t.pop_front();
            speed.pop_front();
            current.pop_front();
            position.pop_front();
            busVoltage.pop_front();
            pwmPercent.pop_front();
            driverTemp.pop_front();
            motorTemp.pop_front();
            multiTurnEncoder.pop_front();
            torque.pop_front();
        }
    }

    void setTimeWindowSeconds(double seconds)
    {
        timeWindowSeconds = seconds;
        trimToWindow();
    }

    void push(double timeS, double speedRps, double currentA, double positionDeg, double busV, double pwm, double drvTemp, double motTemp, double encoder, double torqueVal)
    {
        t.push_back(timeS);
        speed.push_back(speedRps);
        current.push_back(currentA);
        position.push_back(positionDeg);
        busVoltage.push_back(busV);
        pwmPercent.push_back(pwm);
        driverTemp.push_back(drvTemp);
        motorTemp.push_back(motTemp);
        multiTurnEncoder.push_back(encoder);
        torque.push_back(torqueVal);

        trimToWindow();
    }
};

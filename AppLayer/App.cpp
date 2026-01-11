#include "App.hpp"

using namespace AppLayer;

App::App(Hardware &hardware)
: simpleLogger(hardware.usbCom)
, communication(hardware.usbCom, systemData)
, systemDataController(systemData, communication, hardware.flashStorage, userInterface)
, motorControl(hardware.motorPwm, pidController, hardware.adc)
, hwPtr(hardware)
{
}

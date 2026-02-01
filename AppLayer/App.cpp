#include "App.hpp"

using namespace AppLayer;

App::App(Hardware &hardware)
: simpleLogger(hardware.usbCom)
, communication(hardware.usbCom, systemData, userInterface)
, systemDataController(systemData, communication, hardware.flashStorage, userInterface, hardware.drv8316)
, analogProcessor(hardware.adc, systemData)
, motorControl(hardware.motorPwm, pidController, analogProcessor, systemData, hardware.as5047)
, hwPtr(hardware)
{
}

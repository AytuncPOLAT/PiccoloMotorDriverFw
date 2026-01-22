#include "App.hpp"

using namespace AppLayer;

App::App(Hardware &hardware)
: simpleLogger(hardware.usbCom)
, communication(hardware.usbCom, systemData, userInterface)
, systemDataController(systemData, communication, hardware.flashStorage, userInterface, hardware.drv8316)
, motorControl(hardware.motorPwm, pidController, hardware.adc, systemData, hardware.externalQuadEncoder)
, hwPtr(hardware)
{
}

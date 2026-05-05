#include "App.hpp"

using namespace AppLayer;

App::App(Hardware &hardware)
: simpleLogger(hardware.usbCom)
, communication(hardware.usbCom, hardware.rs485, systemData, userInterface)
, analogProcessor(hardware.adc, systemData, hardware.drv8316)
, motorControl(hardware.motorPwm, analogProcessor, systemData, hardware.as5047, hardware.externalQuadEncoder)
, systemDataController(systemData, communication, hardware.flashStorage, userInterface, hardware.drv8316, motorControl)
, telemetry(systemData, analogProcessor, hardware.as5047)
, hwPtr(hardware)
{}

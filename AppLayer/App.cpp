#include "App.hpp"

using namespace AppLayer;

App::App(Hardware &hardware)
: hw(hardware)
, simpleLogger(hardware.usbCom)
, communication(hardware.usbCom, systemData)
, systemDataController(systemData, communication, hw.flashStorage)
{
}

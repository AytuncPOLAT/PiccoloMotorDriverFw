#include "App.hpp"
#include "SimpleLogger.hpp"

using namespace AppLayer;

App::App(Hardware &hardwareRef)
: hardware(hardwareRef)
{
	SimpleLogger simpleLogger(hardware.usbCom);
}

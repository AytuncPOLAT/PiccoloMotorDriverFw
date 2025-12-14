#include "App.hpp"

using namespace AppLayer;

App::App(Hardware &hardware)
: simpleLogger(hardware.usbCom)
, communication(hardware.usbCom)
, hw(hardware)
{
}

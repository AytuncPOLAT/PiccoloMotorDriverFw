#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"
#include "SimpleLogger.hpp"
#include "Communication.hpp"
#include "SystemDataController.hpp"
#include "MotorControl.hpp"

class App
{
public:
	App(Hardware &hardware);
	Common::SystemData systemData;
	AppLayer::SimpleLogger simpleLogger;
	AppLayer::Communication communication;
	AppLayer::SystemDataController systemDataController;
	AppLayer::MotorControl motorControl;

	Hardware& hw;
private:
};

#endif // APP_HPP

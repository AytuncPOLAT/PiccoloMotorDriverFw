#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"
#include "SimpleLogger.hpp"
#include "Communication.hpp"
#include "SystemDataController.hpp"
#include "MotorControl.hpp"
#include "UserInterface.hpp"
#include "PidControl.hpp"

class App
{
public:
	App(Hardware &hardware);
	Common::SystemData systemData;
	AppLayer::SimpleLogger simpleLogger;
	AppLayer::Communication communication;
	AppLayer::SystemDataController systemDataController;
	AppLayer::MotorControl motorControl;
	UserInterface userInterface;
	AppLayer::PidController pidController;

	Hardware& hwPtr;
private:
};

#endif // APP_HPP

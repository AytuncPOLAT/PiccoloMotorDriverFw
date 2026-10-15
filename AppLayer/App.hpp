#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"
#include "SimpleLogger.hpp"
#include "Communication.hpp"
#include "SystemDataController.hpp"
#include "MotorControl.hpp"
#include "UserInterface.hpp"
#include "PidControl.hpp"
#include "AnalogProcessor.hpp"
#include "Telemetry.hpp"
#include "SystemError.hpp"
#include "PowerSupply.hpp"

class App
{
public:
	App(Hardware &hardware);
	Common::SystemData systemData;
	AppLayer::SimpleLogger simpleLogger;
	UserInterface userInterface;
	AppLayer::Communication communication;
	AppLayer::AnalogProcessor analogProcessor;
	AppLayer::MotorControl motorControl;
	AppLayer::SystemError systemError;
	AppLayer::PowerSupply powerSupply;
	AppLayer::SystemDataController systemDataController;
	AppLayer::Telemetry telemetry;
	AppLayer::PidController pidController;

	Hardware& hwPtr;
private:
};

#endif // APP_HPP

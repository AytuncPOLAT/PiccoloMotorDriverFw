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

class App
{
public:
	App(Hardware &hardware);
	Common::SystemData systemData;
	AppLayer::SimpleLogger simpleLogger;
	AppLayer::Communication communication;
	AppLayer::SystemDataController systemDataController;
	AppLayer::AnalogProcessor analogProcessor;
	AppLayer::MotorControl motorControl;
	AppLayer::Telemetry telemetry;
	UserInterface userInterface;
	AppLayer::PidController pidController;

	Hardware& hwPtr;
private:
};

#endif // APP_HPP

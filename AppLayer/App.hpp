#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"
#include "SimpleLogger.hpp"
#include "Communication.hpp"
#include "SystemDataController.hpp"


class App
{
public:
	App(Hardware &hardware);
	Common::SystemData systemData;
	AppLayer::SimpleLogger simpleLogger;
	AppLayer::Communication communication;
	AppLayer::SystemDataController systemDataController;

	Hardware& hw;
private:
};

#endif // APP_HPP

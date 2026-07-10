#ifndef LINK_LAYER_HPP
#define LINK_LAYER_HPP

#include <stdint.h>
#include "Crc16.hpp"
#include "SystemData.hpp"

namespace Common
{
	enum class CMD_TYPE : uint8_t
	{
	    PING = 0,
	    PING_RESPONSE,
	    READ_FROM_DEVICE,
	    WRITE_TO_DEVICE,
	    WRITE_TO_DEVICE_FLASH,
		MOTION_COMMAND,
		READ_REALTIME,
		DRIVER_ARM,
		DRIVER_DISARM,
		POSITION_HOME,
	    CURR_1,
	    CURR_2,
		MOTION_POS_COMMAND,
		MOTION_SPEED_COMMAND,
		MOTION_TORQUE_COMMAND
	};

	struct __attribute__((packed)) CANBusFrame
	{
		uint8_t msgID;
		uint8_t subAddress;
		CMD_TYPE cmd;
		uint8_t flags;
		uint8_t padding;
		uint8_t data[4];
	};

	struct __attribute__((packed)) SerialFrame
	{	
		CANBusFrame canFrame;
		uint16_t checksum;
	};

	class LinkLayer
	{
	public:
        LinkLayer(Common::SystemData &systemDataRef);

	private:
		Common::SystemData& systemData;
	};
}

#endif

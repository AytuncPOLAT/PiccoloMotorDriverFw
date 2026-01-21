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
		READ_REQUEST,
		READ_RESPONSE,
		WRITE_REQUEST,
		WRITE_RESPONSE,
		STORE_REQUEST,
		STORE_RESPONSE,
		SET_POSITION,
		GET_POSITION,
	};

	struct __attribute__((packed)) DataFrame
	{
		CMD_TYPE cmd;
		uint8_t address;
		uint32_t data0;
		uint32_t data1;
		uint32_t data2;
		uint32_t data3;
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

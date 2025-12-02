#include "DebugPrint.hpp"

using namespace AppLayer;

DebugPrint::DebugPrint()
{

}

DebugPrint& DebugPrint::Instance()
{
	static DebugPrint instance;
	return instance;
}

void DebugPrint::Print(uint8_t *data, uint32_t len)
{

}

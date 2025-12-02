#ifndef IUART_H
#define IUART_H

#include <stdint.h>

namespace Common
{
    class IUart
    {
    public:
        //virtual int Transmit(uint8_t* data, uint32_t size) = 0;
        //virtual int Receive(uint8_t* data, uint32_t size) = 0;
		//virtual int ReceiveSlip(uint8_t* data, uint32_t* size) = 0;

		struct Callback
		{
			virtual void OnReceiveCallback(uint8_t *Buf, uint32_t Len) = 0;
		};
        virtual void RegisterOnReceiveCallback(Callback* callBack) = 0;
    };
}

#endif // IUART_H

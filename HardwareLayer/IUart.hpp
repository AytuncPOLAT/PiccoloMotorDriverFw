#ifndef IUART_H
#define IUART_H

#include <stdint.h>

namespace Common
{
    class IUart
    {
    public:
        virtual uint8_t Transmit(uint8_t* data, uint32_t size) = 0;
        virtual uint8_t Receive(uint8_t* data, uint32_t size) = 0;

		struct Callback
		{
			virtual void OnReceiveCallback(uint8_t *Buf, uint32_t Len, void* instance) = 0;
		};
        virtual void RegisterOnReceiveCallback(Callback* callBack) = 0;
        virtual void* GetInstance() = 0;
    };

    // Reusable adapter: binds any member function of T to IUart::Callback.
    // Usage: UartReceiveAdapter<MyClass> cb{*this, &MyClass::OnReceive};
    template<typename T>
    struct UartReceiveAdapter : public IUart::Callback
    {
        T& parent;
        void (T::*method)(uint8_t*, uint32_t);
        UartReceiveAdapter(T& p, void (T::*m)(uint8_t*, uint32_t)) : parent(p), method(m) {}
        void OnReceiveCallback(uint8_t *Buf, uint32_t Len, void* /*instance*/) override
        {
            (parent.*method)(Buf, Len);
        }
    };
}

#endif // IUART_H

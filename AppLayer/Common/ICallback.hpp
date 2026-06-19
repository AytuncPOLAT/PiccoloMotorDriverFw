#ifndef I_CALLBACK_HPP
#define I_CALLBACK_HPP

#include <stdint.h>

namespace Common
{
	class ICallback
	{
	public:
		struct GenericCallback
		{
			virtual void OnCallback(uint8_t arg) = 0;
			// Optional receive-style callback: buffer + length + instance pointer
			virtual void OnReceive(uint8_t* Buf, uint32_t Len, void* instance) { (void)Buf; (void)Len; (void)instance; }
		};
		virtual void RegisterCallback(GenericCallback* callBack) = 0;

	    // Reusable adapter: binds any member function of T to IUart::Callback.
	    // Usage: UartReceiveAdapter<MyClass> cb{*this, &MyClass::OnReceive};
	    template<typename T>
	    struct UartReceiveAdapter : public ICallback::GenericCallback
	    {
	        T& parent;
	        void (T::*method)(uint8_t*, uint32_t);
	        UartReceiveAdapter(T& p, void (T::*m)(uint8_t*, uint32_t)) : parent(p), method(m) {}
	        void OnReceiveCallback(uint8_t *Buf, uint32_t Len, void* /*instance*/) override
	        {
	            (parent.*method)(Buf, Len);
	        }
	    };
	};
}

#endif

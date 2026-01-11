#ifndef USER_INTERFACE
#define USER_INTERFACE

extern "C"
{
#include "main.h"
#include "cmsis_os.h"
}

class HeartBeatState
{
public:
	void Play(uint8_t time);
};

class ErrorState
{
public:
	void Play(uint8_t time);
};

class WarningState
{
public:
	void Play(uint8_t time);
};

class CommActivityState
{
public:
	void Play(uint8_t time);
};

class PingState
{
public:
	void Play(uint8_t time);
};

enum class UiState
{
	Idle = 0,
	HeartBeat = 1,
	Warning = 2,
	Error = 3,
	Ping = 4
};

class UserInterface
{
public:
	UserInterface();
	void SetUiState(UiState newState);
	void CommActivity();
	void PingActivity();
	void Ping();
	void Init();

private:
	HeartBeatState heartBeat;
	WarningState warning;
	ErrorState error;
	CommActivityState commAct;
	PingState pingAct;

	BaseType_t uiTaskHandle;
	static void UiTask(void *argument);

	uint8_t tenMsCounter = 0;

	UiState state = UiState::Idle;
	UiState oldState = UiState::Idle;
};

#endif //USER_INTERFACE

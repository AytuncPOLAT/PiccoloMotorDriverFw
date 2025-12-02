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
	void
	Play();
};

class ErrorState
{
public:
	void
	Play();
};

class WarningState
{
public:
	void
	Play();
};

class CommActivityState
{
public:
	void
	Play();
};

enum class UiState
{
	Idle = 0, HeartBeat = 1, Warning = 2, Error = 3
};

class UserInterface
{
public:
	UserInterface();
	void
	SetUiState(UiState newState);
	void
	CommActivity();

private:
	HeartBeatState heartBeat;
	WarningState warning;
	ErrorState error;
	CommActivityState commAct;

	const osThreadAttr_t uiTaskattributes =
	{ .name = "uiTask", .stack_size = 128 * 4, .priority =
			(osPriority_t) osPriorityNormal, };

	BaseType_t uiTaskHandle;
	static void UiTask(void *argument);

	UiState state = UiState::Idle;
};

#endif //USER_INTERFACE

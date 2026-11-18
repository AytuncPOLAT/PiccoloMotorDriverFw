#include "UserInterface.hpp"
#include "BoardConfig/BoardConfig.h"

void HeartBeatState::Play(uint8_t time)
{
	if(time == 0)
		HAL_GPIO_WritePin(BOARD_STATUS_HEARTBEAT_PORT, BOARD_STATUS_HEARTBEAT_PIN, GPIO_PIN_SET);
	if(time == 2)
		HAL_GPIO_WritePin(BOARD_STATUS_HEARTBEAT_PORT, BOARD_STATUS_HEARTBEAT_PIN, GPIO_PIN_RESET);
	if(time == 18)
		HAL_GPIO_WritePin(BOARD_STATUS_HEARTBEAT_PORT, BOARD_STATUS_HEARTBEAT_PIN, GPIO_PIN_SET);
	if(time == 20)
		HAL_GPIO_WritePin(BOARD_STATUS_HEARTBEAT_PORT, BOARD_STATUS_HEARTBEAT_PIN, GPIO_PIN_RESET);
}

void ErrorState::Play(uint8_t time)
{
	if ((time % 50) < 25)
		HAL_GPIO_WritePin(BOARD_STATUS_ERROR_PORT, BOARD_STATUS_ERROR_PIN, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(BOARD_STATUS_ERROR_PORT, BOARD_STATUS_ERROR_PIN, GPIO_PIN_RESET);
}

void WarningState::Play(uint8_t time)
{
	if(time == 24)
		HAL_GPIO_WritePin(BOARD_STATUS_HEARTBEAT_PORT, BOARD_STATUS_HEARTBEAT_PIN, GPIO_PIN_SET);
	if(time == 49)
		HAL_GPIO_WritePin(BOARD_STATUS_HEARTBEAT_PORT, BOARD_STATUS_HEARTBEAT_PIN, GPIO_PIN_RESET);
}

void CommActivityState::Play(uint8_t time)
{
	HAL_GPIO_WritePin(BOARD_STATUS_ERROR_PORT, BOARD_STATUS_ERROR_PIN, GPIO_PIN_SET);
	osDelay(5);
	HAL_GPIO_WritePin(BOARD_STATUS_ERROR_PORT, BOARD_STATUS_ERROR_PIN, GPIO_PIN_RESET);
}

void PingState::Play(uint8_t time)
{
	HAL_GPIO_WritePin(BOARD_STATUS_PING_PORT, BOARD_STATUS_PING_PIN, GPIO_PIN_SET);
	osDelay(50);
	HAL_GPIO_WritePin(BOARD_STATUS_PING_PORT, BOARD_STATUS_PING_PIN, GPIO_PIN_RESET);
}

UserInterface::UserInterface()
{

}

void UserInterface::Init()
{
	uiTaskHandle = xTaskCreate(this->UiTask, "UI_TASK", 128 * 4, (void*) this,
			24, NULL);
}

void UserInterface::UiTask(void *argument)
{
	UserInterface *objectHandle = static_cast<UserInterface*>(argument);
	while (1)
	{
		switch (objectHandle->state)
		{
		case UiState::Idle:
			break;

		case UiState::HeartBeat:
			objectHandle->heartBeat.Play(objectHandle->tenMsCounter);
			break;

		case UiState::Warning:
			objectHandle->warning.Play(objectHandle->tenMsCounter);

			break;
		case UiState::Error:
			objectHandle->error.Play(objectHandle->tenMsCounter);
			break;

		case UiState::Ping:
			objectHandle->pingAct.Play(objectHandle->tenMsCounter);
			objectHandle->state = UiState::HeartBeat;
			break;

		case UiState::Comm:
			objectHandle->commAct.Play(objectHandle->tenMsCounter);
			objectHandle->state = objectHandle->oldState;
			break;
		}

		osDelay(10);
		if(objectHandle->tenMsCounter++ > 99)
		{
			objectHandle->tenMsCounter = 0;
		}

	}
}

void UserInterface::SetUiState(UiState newState)
{
	state = newState;
	oldState = newState;
	tenMsCounter = 0;
}

void UserInterface::CommActivity()
{
	if(state != UiState::Comm)
	{
		oldState = state;
		tenMsCounter = 0;
		state = UiState::Comm;
	}
}

void UserInterface::PingActivity()
{
	oldState = state;
	tenMsCounter = 0;
	state = UiState::Ping;
}

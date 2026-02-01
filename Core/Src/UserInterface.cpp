#include "UserInterface.hpp"

void HeartBeatState::Play(uint8_t time)
{
	if(time == 0)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
	if(time == 1)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
	if(time == 19)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
	if(time == 20)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
}

void ErrorState::Play(uint8_t time)
{
	if(time == 49)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	if(time == 99)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
}

void WarningState::Play(uint8_t time)
{
	if(time == 24)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
	if(time == 49)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
}

void CommActivityState::Play(uint8_t time)
{
	if(time == 0)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
	if(time == 2)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
}

void PingState::Play(uint8_t time)
{
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
	osDelay(50);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
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
	tenMsCounter = 0;
}

void UserInterface::CommActivity()
{
	//commAct.Play();
}

void UserInterface::PingActivity()
{
	oldState = state;
	tenMsCounter = 0;
	state = UiState::Ping;
}

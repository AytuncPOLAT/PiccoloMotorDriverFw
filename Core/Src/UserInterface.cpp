#include "UserInterface.hpp"

void HeartBeatState::Play()
{
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
	osDelay(5);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
	osDelay(195);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
	osDelay(5);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
	osDelay(795);
}

void ErrorState::Play()
{
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	osDelay(250);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
	osDelay(250);
}

void WarningState::Play()
{
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
	osDelay(500);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
	osDelay(500);
}

void CommActivityState::Play()
{
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
	osDelay(10);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
}

UserInterface::UserInterface()
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
			objectHandle->heartBeat.Play();
			break;

		case UiState::Warning:
			objectHandle->warning.Play();

			break;
		case UiState::Error:
			objectHandle->error.Play();
			break;

		}

	}
}

void UserInterface::SetUiState(UiState newState)
{
	state = newState;
}

void UserInterface::CommActivity()
{
	commAct.Play();
}

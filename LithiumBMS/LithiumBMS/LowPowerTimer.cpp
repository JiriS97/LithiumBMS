#include "LowPowerTimer.h"

LowPowerTimer::LowPowerTimer() {
	//config clock
	EnableLSI();
	  
	__HAL_RCC_LPTIM1_CLK_ENABLE();
	
	handle_.Instance = LPTIM1;
  
	handle_.Init.Clock.Source       = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
	handle_.Init.Clock.Prescaler    = LPTIM_PRESCALER_DIV128; //37kHz / 128, 16bit counter -> 226s max, 3.5ms accuracy
	handle_.Init.Trigger.Source     = LPTIM_TRIGSOURCE_SOFTWARE;
	handle_.Init.Trigger.ActiveEdge = LPTIM_ACTIVEEDGE_RISING;
	handle_.Init.CounterSource      = LPTIM_COUNTERSOURCE_INTERNAL;
	handle_.Instance = LPTIM1;

	HAL_LPTIM_Init(&handle_); 
	
	HAL_NVIC_SetPriority(LPTIM1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
}

void LowPowerTimer::EnableLSI() {
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
  
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
	PeriphClkInitStruct.LptimClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

void LowPowerTimer::SetTimeoutAfter(float seconds){
	done_ = false;
	float compareVal = (37000.0f / 128.0f)*seconds - 1;
	HAL_LPTIM_SetOnce_Start_IT(&handle_, 0xFFFF, compareVal);
}

void LowPowerTimer::SetTimeoutDone() {
	done_ = 1;
	HAL_LPTIM_TimeOut_Stop_IT(&handle_);
}

void LowPowerTimer::LPTIM_ISR() {
	HAL_LPTIM_IRQHandler(&handle_);
}


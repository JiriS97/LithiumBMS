#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include "STM32_Cpp/STM32Cpp.h"
#include "STM32_Cpp/GpioPin.h"
#include "STM32_Cpp/CircularBuffer.h"

class Communication{
public:
	Communication();
	
	void StartRX() {
		HAL_UART_Receive_DMA(&huart1, (uint8_t*)buffer.GetAddressForDMA(), BUFFER_SIZE);
	}
	
	void HandleCommands();
	void DMA_ISR() {
		HAL_DMA_IRQHandler(&hdma_usart1_rx);
	}
	
private:
	void SendResponse(char *str);
	
	static const int BUFFER_SIZE = 256;
	UART_HandleTypeDef huart1;
	DMA_HandleTypeDef hdma_usart1_rx;
	CircularBuffer<char> buffer;
};

#endif

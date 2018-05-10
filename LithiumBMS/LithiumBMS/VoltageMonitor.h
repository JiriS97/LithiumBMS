#ifndef VOLTAGE_MONITOR_H_
#define VOLTAGE_MONITOR_H_

#include "STM32_Cpp/STM32Cpp.h"
#include "STM32_Cpp/GpioPin.h"
#include "STM32_Cpp/Delay.h"
#include "STM32_Cpp/PeriphUtils.h"

class VoltageMonitor{
public:
	VoltageMonitor();
	void UpdateAll();
	uint16_t GetCellMV(int accuNum);
	float GetTempDegC();
	
private:
	static const int ADC_VDD = 3000;
	uint16_t voltages_[7];
	GpioPin OpAmpPower_, tempNShdn_;
	ADC_HandleTypeDef adcHandle_;
};

#endif

#include  "VoltageMonitor.h"

VoltageMonitor::VoltageMonitor()
	: OpAmpPower_(PC14)
	, tempNShdn_(PC15) {
	PeriphUtils::EnableClock(ADC1);
	GpioPin(PA0).Analog();       //temp
	GpioPin(PA1).Analog();       //aku1..6
	GpioPin(PA2).Analog();
	GpioPin(PA3).Analog();
	GpioPin(PA4).Analog();
	GpioPin(PA5).Analog();
	GpioPin(PA6).Analog();
	OpAmpPower_.Output();
	tempNShdn_.Output();
	OpAmpPower_ = 0;  	//shutdown all analog periphs
	tempNShdn_ = 0;
	Delay::Init();

	adcHandle_.Instance = ADC1;

	adcHandle_.Init.OversamplingMode = DISABLE;

	adcHandle_.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
	adcHandle_.Init.LowPowerAutoPowerOff = ENABLE;
	adcHandle_.Init.LowPowerFrequencyMode = DISABLE;
	adcHandle_.Init.LowPowerAutoWait = ENABLE;

	adcHandle_.Init.Resolution = ADC_RESOLUTION_12B;
	adcHandle_.Init.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;
	adcHandle_.Init.ScanConvMode = ADC_SCAN_DIRECTION_BACKWARD;
	adcHandle_.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adcHandle_.Init.ContinuousConvMode = DISABLE;
	adcHandle_.Init.DiscontinuousConvMode = DISABLE;
	adcHandle_.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	adcHandle_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	adcHandle_.Init.DMAContinuousRequests = DISABLE;

	HAL_ADC_Init(&adcHandle_);
	HAL_ADCEx_Calibration_Start(&adcHandle_, ADC_SINGLE_ENDED);

	ADC_ChannelConfTypeDef chConfig;
	chConfig.Channel = ADC_CHANNEL_0;      //temp
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);

	chConfig.Channel = ADC_CHANNEL_1;  	 //accu
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);
	chConfig.Channel = ADC_CHANNEL_2;
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);
	chConfig.Channel = ADC_CHANNEL_3;
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);
	chConfig.Channel = ADC_CHANNEL_4;
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);
	chConfig.Channel = ADC_CHANNEL_5;
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);
	chConfig.Channel = ADC_CHANNEL_6;
	HAL_ADC_ConfigChannel(&adcHandle_, &chConfig);
}

void VoltageMonitor::UpdateAll() {
	OpAmpPower_ = 1;
	tempNShdn_ = 1;
	Delay::Ms(2);

	HAL_ADC_Start(&adcHandle_);
	for (int i = 0; i < 7; i++) {
		HAL_ADC_PollForConversion(&adcHandle_, 10);
		if ((HAL_ADC_GetState(&adcHandle_) & HAL_ADC_STATE_REG_EOC) == HAL_ADC_STATE_REG_EOC) {
			voltages_[i] = (HAL_ADC_GetValue(&adcHandle_)*ADC_VDD) / 4096.0;
			if (i != 6) voltages_[i] *= 2.0606; //0-5 aku, 6 temperature sensor
		}
	}

	OpAmpPower_ = 0;
	tempNShdn_ = 0;
}


uint16_t VoltageMonitor::GetCellMV(int id) {
	return voltages_[id]; //0-5 aku
}


float VoltageMonitor::GetTempDegC() {
	return (voltages_[6] - 500) / 10.0; //(voltage[mV] - 500)/10
}

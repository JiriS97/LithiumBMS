#include "Communication.h"
#include "string.h"
#include "DataTypes.h"
#include "Eeprom.h"

extern Configuration moduleConfig;
extern MeasuredData moduleData;
extern GpioPin PowerSwitch;

Communication::Communication() : buffer(BUFFER_SIZE) {
	GpioPin(PA9).Alternate(4);
	GpioPin(PA10).Alternate(4);

	PeriphUtils::EnableClock(DMA1);
	PeriphUtils::EnableClock(USART1);

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(&huart1);

	hdma_usart1_rx.Instance = DMA1_Channel3;
	hdma_usart1_rx.Init.Request = DMA_REQUEST_3;
	hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
	hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
	HAL_DMA_Init(&hdma_usart1_rx);

	__HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

	HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

void Communication::HandleCommands() {
	buffer.UpdateWriteIndexDMA(BUFFER_SIZE - hdma_usart1_rx.Instance->CNDTR);
	int indexOfEnd;
	if (indexOfEnd = buffer.Contains("\n", 1), indexOfEnd != -1) {
		char receivedCommand[128] = { 0, };
		char commandResponse[128] = { 0, };
		buffer.Get(receivedCommand, (indexOfEnd + 1) % 128); //get max 128bytes
															 //current state
		if (strstr(receivedCommand, "AT?")) {
			SendResponse("OK\r\n");
		}
		else if (strstr(receivedCommand, "AT+VPACK?")) {
			sprintf(commandResponse, "+VPACK:%g\r\n", moduleData.packVoltage);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+I?")) {
			sprintf(commandResponse, "+I:%g\r\n", moduleData.current);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+P?")) {
			sprintf(commandResponse, "+P:%g\r\n", moduleData.power);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+T?")) {
			sprintf(commandResponse, "+T:%g\r\n", moduleData.temperature);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+NCELLS?")) {
			sprintf(commandResponse, "+NCELLS:%d\r\n", moduleData.nCells);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+VCELLS?")) {
			sprintf(commandResponse,
				"+VCELLS:%g,%g,%g,%g,%g,%g\r\n",
				moduleData.cellVoltage[0],
				moduleData.cellVoltage[1],
				moduleData.cellVoltage[2],
				moduleData.cellVoltage[3],
				moduleData.cellVoltage[4],
				moduleData.cellVoltage[5]);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+BAL?")) {
			sprintf(commandResponse,
				"+BAL:%d,%d,%d,%d,%d,%d\r\n",
				moduleData.balancerStatus[0] ? 1 : 0,
				moduleData.balancerStatus[1] ? 1 : 0,
				moduleData.balancerStatus[2] ? 1 : 0,
				moduleData.balancerStatus[3] ? 1 : 0,
				moduleData.balancerStatus[4] ? 1 : 0,
				moduleData.balancerStatus[5] ? 1 : 0);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+HWFUSE?")) {
			sprintf(commandResponse, "+HWFUSE:%d\r\n", (uint8_t)moduleData.hwFuse);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+SWFUSE?")) {
			sprintf(commandResponse, "+SWFUSE:%d\r\n", (uint8_t)moduleData.swFuse);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+STATUS?")) {
			sprintf(commandResponse, "+STATUS:%d\r\n", (uint8_t)moduleData.state);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+PERCENT?")) {
			sprintf(commandResponse, "+PERCENT:%g\r\n", moduleData.packPercentage);
			SendResponse(commandResponse);
		}
		//configuration query
		else if (strstr(receivedCommand, "AT+VCUTOFF?")) {
			sprintf(commandResponse, "+VCUTOFF:%g,%g\r\n", moduleConfig.CellVoltageCutoff.lower, moduleConfig.CellVoltageCutoff.higher);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+ICUTOFF?")) {
			sprintf(commandResponse, "+ICUTOFF:%g\r\n", moduleConfig.currentCutoff);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+TCUTOFF?")) {
			sprintf(commandResponse, "+TCUTOFF:%g\r\n", moduleConfig.tempCutoff);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+VBAL?")) {
			sprintf(commandResponse, "+VBAL:%g,%g\r\n", moduleConfig.BalancerVoltages.onDelta, moduleConfig.BalancerVoltages.offDelta);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+RSENSE?")) {
			sprintf(commandResponse, "+RSENSE:%g\r\n", moduleConfig.rSense);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+LED?")) {
			sprintf(commandResponse, "+LED:%d\r\n", moduleConfig.ledEnable ? 1 : 0);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+BTN?")) {
			sprintf(commandResponse, "+BTN:%d\r\n", moduleConfig.buttonEnable ? 1 : 0);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+EBAL?")) {
			sprintf(commandResponse, "+EBAL:%d\r\n", moduleConfig.balancerEnable ? 1 : 0);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+VSTIME?")) {
			sprintf(commandResponse, "+VSTIME:%g\r\n", moduleConfig.voltageSenseTime);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+ISTIME?")) {
			sprintf(commandResponse, "+ISTIME:%g\r\n", moduleConfig.currentSenseTime);
			SendResponse(commandResponse);
		}
		else if (strstr(receivedCommand, "AT+SWFAUTORES?")) {
			sprintf(commandResponse, "+SWFAUTORES:%d\r\n", moduleConfig.swFuseAutoResetEnable ? 1 : 0);
			SendResponse(commandResponse);
		}
		//configuration set
		else if (strstr(receivedCommand, "AT+VCUTOFF=")) {
			float readLow, readHigh;
			if (sscanf(receivedCommand, "AT+VCUTOFF=%f,%f", &readLow, &readHigh) == 2) {
				moduleConfig.CellVoltageCutoff.lower = readLow;
				moduleConfig.CellVoltageCutoff.higher = readHigh;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+ICUTOFF=")) {
			float readI;
			if (sscanf(receivedCommand, "AT+ICUTOFF=%f", &readI) == 1) {
				moduleConfig.currentCutoff = readI;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+TCUTOFF=")) {
			float readT;
			if (sscanf(receivedCommand, "AT+TCUTOFF=%f", &readT) == 1) {
				moduleConfig.tempCutoff = readT;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+VBAL=")) {
			float readOn, readOff;
			if (sscanf(receivedCommand, "AT+VBAL=%f,%f", &readOn, &readOff) == 2) {
				moduleConfig.BalancerVoltages.onDelta = readOn;
				moduleConfig.BalancerVoltages.offDelta = readOff;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+RSENSE=")) {
			float readR;
			if (sscanf(receivedCommand, "AT+RSENSE=%f", &readR) == 1) {
				moduleConfig.rSense = readR;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+LED=")) {
			int ledE;
			if (sscanf(receivedCommand, "AT+LED=%d", &ledE) == 1) {
				if (ledE == 1 || ledE == 0) {
					moduleConfig.ledEnable = ledE;
					SendResponse("OK\r\n");
				}
				else SendResponse("ERROR\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+BTN=")) {
			int btnE;
			if (sscanf(receivedCommand, "AT+BTN=%d", &btnE) == 1) {
				if (btnE == 1 || btnE == 0) {
					moduleConfig.buttonEnable = btnE;
					SendResponse("OK\r\n");
				}
				else SendResponse("ERROR\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+EBAL=")) {
			int balE;
			if (sscanf(receivedCommand, "AT+EBAL=%d", &balE) == 1) {
				if (balE == 1 || balE == 0) {
					moduleConfig.balancerEnable = balE;
					SendResponse("OK\r\n");
				}
				else SendResponse("ERROR\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+VSTIME=")) {
			float readVstime;
			if (sscanf(receivedCommand, "AT+VSTIME=%f", &readVstime) == 1) {
				moduleConfig.voltageSenseTime = readVstime;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+ISTIME=")) {
			float readIstime;
			if (sscanf(receivedCommand, "AT+ISTIME=%f", &readIstime) == 1) {
				moduleConfig.currentSenseTime = readIstime;
				SendResponse("OK\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		else if (strstr(receivedCommand, "AT+SWFAUTORES=")) {
			int autoresE;
			if (sscanf(receivedCommand, "AT+SWFAUTORES=%d", &autoresE) == 1) {
				if (autoresE == 1 || autoresE == 0) {
					moduleConfig.swFuseAutoResetEnable = autoresE;
					SendResponse("OK\r\n");
				}
				else SendResponse("ERROR\r\n");
			}
			else SendResponse("ERROR\r\n");
		}
		//commands
		else if (strstr(receivedCommand, "AT+SWFRES")) {
			moduleData.swFuse = FuseState::OK;
			PowerSwitch = 0;       //on
			moduleData.outputOn = 1;
			SendResponse("OK\r\n");
		}
		else if (strstr(receivedCommand, "AT+SAVE")) {
			Eeprom::SaveConfig(moduleConfig);
			SendResponse("OK\r\n");
		}
		else if (strstr(receivedCommand, "AT+LOAD")) {
			Eeprom::LoadConfig(&moduleConfig);
			SendResponse("OK\r\n");
		}
		else {
			SendResponse("ERROR\r\n");
		}
	}
}


void Communication::SendResponse(char *str) {
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

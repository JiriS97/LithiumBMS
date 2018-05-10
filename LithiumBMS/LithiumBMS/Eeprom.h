#ifndef EEPROM_H_
#define EEPROM_H_

#include "stm32l0xx_hal.h"
#include "DataTypes.h"

namespace Eeprom {
	
	bool IsConfigAvailable();
	void SaveConfig(Configuration cfg);
	void LoadConfig(Configuration *cfg);
	
	bool Write(int dataIndex, uint32_t data);
	bool Write(int dataIndex, uint32_t *data, int numOf32bits);
	void Read(int dataIndex, uint32_t *data);
	void Read(int dataIndex, uint32_t *data, int numOf32bits);
	
	const uint32_t EEPROM_START_ADDR = 0x08080000;
	const uint32_t CONFIG_SIGNATURE = 0x12AB34CD;
}

#endif

#include "Eeprom.h"
#include "string.h"

bool Eeprom::Write(int dataIndex, uint32_t data) {
	if (HAL_FLASH_Unlock() != HAL_OK) return false;
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, EEPROM_START_ADDR + 4 * dataIndex, data) != HAL_OK) return false;
	if(HAL_FLASH_Lock() != HAL_OK) return false;
	return true;
}

bool Eeprom::Write(int dataIndex, uint32_t *data, int numOf32bits) {
	if (HAL_FLASH_Unlock() != HAL_OK) return false;
	for (int i = 0; i < numOf32bits; i++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, EEPROM_START_ADDR + 4 * dataIndex++, data[i]) != HAL_OK) return false;
	}
	if (HAL_FLASH_Lock() != HAL_OK) return false;
	return true;
}

void Eeprom::Read(int dataIndex, uint32_t *data) {
	*data = *(uint32_t*)(EEPROM_START_ADDR + 4 * dataIndex);
}

void Eeprom::Read(int dataIndex, uint32_t *data, int numOf32bits) {
	for (int i = 0; i < numOf32bits; i++) {
		*data = *(uint32_t*)(EEPROM_START_ADDR + 4 * dataIndex++);
		data++;
	}
}

bool Eeprom::IsConfigAvailable() {
	uint32_t readSign;
	Eeprom::Read(0, &readSign);
	if (readSign == CONFIG_SIGNATURE) return true;
	return false;
}


void Eeprom::SaveConfig(Configuration cfg) {
	uint32_t arr[64] = { 0, };
	
	int cfgSizeIn32b = sizeof(Configuration) / 4;  //align to 4B
	if (cfgSizeIn32b % 4) cfgSizeIn32b++;
	
	memcpy(arr, &cfg, sizeof(Configuration));
	Write(0, arr, cfgSizeIn32b);
}


void Eeprom::LoadConfig(Configuration *cfg) {
	uint32_t arr[64] = { 0, };
	
	int cfgSizeIn32b = sizeof(Configuration) / 4;   //align to 4B
	if(cfgSizeIn32b % 4) cfgSizeIn32b++;
	
	Read(0, arr, cfgSizeIn32b);
	
	memcpy(cfg, &arr, sizeof(Configuration));
}

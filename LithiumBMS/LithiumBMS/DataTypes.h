#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include <stdint.h>

struct Configuration {
	uint32_t SIGNATURE_;
	struct {
		float higher, lower;
	} CellVoltageCutoff;
	float currentCutoff;
	float tempCutoff;
	struct {
		float onDelta, offDelta;
	} BalancerVoltages;
	float rSense;
	bool ledEnable;
	bool buttonEnable;
	bool balancerEnable;
	float voltageSenseTime;
	float currentSenseTime;
	bool swFuseAutoResetEnable;
};

enum class FuseState {
	OK = 0,
	OVER_CURRENT = 1,
	UNDER_VOLTAGE = 2,
	OVER_VOLTAGE = 3,
	OVER_TEMPERATURE = 4
};

enum class BoardState {
	OUTPUT_OFF = 0,
	DISCHARGING = 1,
	CHARGING = 2,
	REBALANCING = 3,
	CHARGING_FINISHED = 4,
	CAN_NOT_CHARGE_V_TOO_LOW = 5,
	CAN_NOT_CHARGE_V_TOO_HIGH = 6
};

struct MeasuredData {
	float packVoltage;
	float current;
	float power;
	float temperature;
	int nCells;
	FuseState hwFuse, swFuse;
	float cellVoltage[6];
	bool balancerStatus[6];
	bool outputOn;
	
	float cellVoltageSum; //sum of cellVoltages
	float cellVoltageMin; //min sensed voltage
	float packPercentage;
	BoardState state;
};

#endif
#include "STM32_Cpp/STM32Cpp.h"
#include "STM32_Cpp/GpioPin.h"
#include "STM32_Cpp/Delay.h"
#include "STM32_Cpp/I2cHardware.h"
#include "STM32_Cpp/INA219.h"
#include "VoltageMonitor.h"
#include "Balancer.h"
#include "Communication.h"
#include "DataTypes.h"
#include "Eeprom.h"
#include "LowPowerTimer.h"
#include <cmath>

//pins
GpioPin ExtOn(PB0, PinMode::INPUT);
GpioPin Button(PB1, PinMode::INPUT);
GpioPin Led(PB3, PinMode::OUTPUT);
GpioPin PowerSwitch(PA15, PinMode::OUTPUT);

//global objects
Communication SerialLink;
LowPowerTimer WakeupTimer;
I2cHardware InaBus(I2C1, PB6, PB7, 1, 0x00B1112E);
INA219 Monitor(InaBus);
Balancer BatteryBalancer;
VoltageMonitor BatteryMonitor;

//interrupts and callbacks
extern "C" void DMA1_Channel2_3_IRQHandler(void) {
	SerialLink.DMA_ISR();
}

extern "C" void LPTIM1_IRQHandler() {
	WakeupTimer.LPTIM_ISR();
}

extern "C" void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
	WakeupTimer.SetTimeoutDone();
}

//global constants
const float ONE_LIION_CELL_VOLTAGE_HALF = 3.5 / 2;
const float TURN_ON_VOLTAGE_TRESHOLD = 1.0;
const int TURN_OFF_DELAY = 1000;
const int TURN_ON_DELAY = 5000;

//module data and config
Configuration moduleConfig;
MeasuredData moduleData;

//This function initializes board config, when none was found in EEPROM
void InitDefaultConfig() {
	moduleConfig.SIGNATURE_ = Eeprom::CONFIG_SIGNATURE;
	moduleConfig.balancerEnable = true;
	moduleConfig.buttonEnable = true;
	moduleConfig.currentCutoff = 16;
	moduleConfig.currentSenseTime = 0.25;
	moduleConfig.ledEnable = true;
	moduleConfig.rSense = 0.01;
	moduleConfig.swFuseAutoResetEnable = false;
	moduleConfig.tempCutoff = 40;
	moduleConfig.voltageSenseTime = 1;
	
	moduleConfig.CellVoltageCutoff.higher = 4.2;
	moduleConfig.CellVoltageCutoff.lower = 3.4;
	
	moduleConfig.BalancerVoltages.onDelta = 0.25;
	moduleConfig.BalancerVoltages.offDelta = 0.1;
}

//Read cell voltages, compute minimum and number of cells
void UpdateCellVoltages() {
	BatteryMonitor.UpdateAll();  //ADC values
	moduleData.temperature = BatteryMonitor.GetTempDegC();
	
	moduleData.nCells = 0;
	moduleData.cellVoltageSum = 0;
	moduleData.cellVoltageMin = 4.2;
	moduleData.packPercentage = 0;
	for (int i = 0; i < 6; i++) {
		moduleData.cellVoltage[i] = BatteryMonitor.GetCellMV(i) / 1000.0f;
		moduleData.balancerStatus[i] = BatteryBalancer.GetStatus(i);
		if (moduleData.cellVoltage[i] > 2.5) {
			moduleData.nCells++;   //voltage higher than 2.5V -> cell present
			moduleData.packPercentage += (moduleData.cellVoltage[i] - moduleConfig.CellVoltageCutoff.lower) / (moduleConfig.CellVoltageCutoff.higher - moduleConfig.CellVoltageCutoff.lower);
			if(moduleData.cellVoltage[i] < moduleData.cellVoltageMin) moduleData.cellVoltageMin = moduleData.cellVoltage[i];  //find minimum
		}
		moduleData.cellVoltageSum += moduleData.cellVoltage[i];
	}
	if(moduleData.nCells) moduleData.packPercentage = (100.0f * moduleData.packPercentage) / moduleData.nCells; //calculate pack charge
	else moduleData.packPercentage = 0;
	if(moduleData.packPercentage < 0) moduleData.packPercentage = 0;
	if (moduleData.packPercentage > 100) moduleData.packPercentage = 100;
}

//read data from INA219
void UpdateDataINA() {
	Monitor.SetSystemMode(INA219::SystemMode::MODE_SANDBVOLT_TRIGGERED);
	while (!Monitor.IsDataReady());
	moduleData.packVoltage = Monitor.ReadBusVoltage_V();
	moduleData.current = (Monitor.ReadShuntVoltage_mV() / moduleConfig.rSense) / 1000.0f;
	moduleData.power = moduleData.packVoltage * moduleData.current;
	Monitor.SetSystemMode(INA219::SystemMode::MODE_POWERDOWN);
}

bool IsUnderVoltage() {
	for (int i = 0; i < moduleData.nCells; i++) {
		if (moduleData.cellVoltage[i] <= moduleConfig.CellVoltageCutoff.lower) return true;
	}
	return 0;
}

bool IsOverVoltage() {
	for (int i = 0; i < moduleData.nCells; i++) {
		if (moduleData.cellVoltage[i] >= moduleConfig.CellVoltageCutoff.higher) return true;
	}
	return 0;
}

bool IsBalancerOn() {
	for (int i = 0; i < moduleData.nCells; i++) {
		if (BatteryBalancer.GetStatus(i)) return true;
	}
	return 0;
}

//complete logic of BMS board
//THIS FUNCTION WILL BE REWRITTEN/SPLIT IN FUTURE VERSION
void ProcessData() {
	//balancer
	for (int i = 0; i < moduleData.nCells; i++) {
		if (moduleData.cellVoltage[i] >(moduleData.cellVoltageMin + moduleConfig.BalancerVoltages.onDelta)) BatteryBalancer.SetAku(i, true);
		if (moduleData.cellVoltage[i] < (moduleData.cellVoltageMin + moduleConfig.BalancerVoltages.offDelta)) BatteryBalancer.SetAku(i, false);
	}

	//output on/off
	static int startTime = 0;
	if (moduleData.swFuse == FuseState::OK || moduleData.swFuse == FuseState::UNDER_VOLTAGE) {//fuse ok
			if (moduleData.packVoltage > moduleData.nCells*ONE_LIION_CELL_VOLTAGE_HALF && moduleData.state != BoardState::REBALANCING && moduleData.state != BoardState::CHARGING_FINISHED) {
				 //voltage on output terminals and not rebalancing accu
				if (moduleData.packVoltage>(moduleData.cellVoltageSum + 0.2)) { //voltage higher than cells
					if (moduleData.packVoltage < (moduleData.nCells*moduleConfig.CellVoltageCutoff.higher + 0.5)) { //voltage on input in range
						moduleData.state = BoardState::CHARGING;
						PowerSwitch = 0;     //on
						moduleData.outputOn = 1;
						startTime = HAL_GetTick();
					}
					else { //voltage on input too high for charging
						moduleData.state = BoardState::CAN_NOT_CHARGE_V_TOO_HIGH;
						PowerSwitch = 1;      //off
						moduleData.outputOn = 0;
					}
				}
				else if((moduleData.state == BoardState::OUTPUT_OFF && moduleData.packVoltage > moduleData.nCells*moduleConfig.CellVoltageCutoff.higher/2) || 
					    (moduleData.state == BoardState::CHARGING && moduleData.current < 0.005)) {
					//the voltage is too low, or while charging, almostost no current flows
					moduleData.state = BoardState::CAN_NOT_CHARGE_V_TOO_LOW;
					PowerSwitch = 1;       //off
					moduleData.outputOn = 0;
				}
			}
			
			//load connected to output terminals
			if(moduleData.swFuse == FuseState::OK && moduleData.packVoltage < TURN_ON_VOLTAGE_TRESHOLD) {
				 //discharging
				moduleData.state = BoardState::DISCHARGING;
				PowerSwitch = 0;     //on
				moduleData.outputOn = 1;
				startTime = HAL_GetTick();
			}

		//no current is flowing and time since turn on is more then TURN_OFF_DELAY
		if (moduleData.outputOn && (std::abs(moduleData.current) < 0.005) && (HAL_GetTick() > (startTime + TURN_OFF_DELAY))) {
			moduleData.state = BoardState::OUTPUT_OFF;
			PowerSwitch = 1;   //off
			moduleData.outputOn = 0;
		}

		if (moduleData.state == BoardState::CHARGING && IsOverVoltage()) { //was charging and at least one cell is full
			if(IsBalancerOn()) moduleData.state = BoardState::REBALANCING;
			else moduleData.state = BoardState::CHARGING_FINISHED;
			PowerSwitch = 1;   //off
			moduleData.outputOn = 0;
		}

		if (moduleData.state == BoardState::REBALANCING && !IsBalancerOn()) { //finished rebalancing
			moduleData.state = BoardState::OUTPUT_OFF;
		}
	}

	//check HW fuse
	if (moduleData.outputOn && (moduleData.packVoltage - moduleData.cellVoltageSum) < -2) moduleData.hwFuse = FuseState::OVER_CURRENT; //hw fuse is gone
	else moduleData.hwFuse = FuseState::OK;  //hw fuse ok

	//check SW fuse
	static int stopTime = 0;
	bool SW_overCurrent = moduleData.current >= moduleConfig.currentCutoff;
	bool SW_overTemp = moduleData.temperature >= moduleConfig.tempCutoff;
	int SW_underVoltage = IsUnderVoltage();
	if (SW_overCurrent || SW_overTemp || SW_underVoltage) {
		//sw fuse trigerred
		if (SW_overCurrent) moduleData.swFuse = FuseState::OVER_CURRENT;
		else if (SW_underVoltage) moduleData.swFuse = FuseState::UNDER_VOLTAGE;
		//else if (SW_under_overVoltage == 2) moduleData.swFuse = FuseState::OVER_VOLTAGE;
		else if (SW_overTemp) moduleData.swFuse = FuseState::OVER_TEMPERATURE;
		moduleData.state = BoardState::OUTPUT_OFF;
		PowerSwitch = 1;  		//off
		moduleData.outputOn = 0;
		stopTime = HAL_GetTick();
	}
	else if (moduleData.swFuse != FuseState::OK) { //fuse trigered, data ok now, can turn on
		if (moduleConfig.buttonEnable) { //reset with button
			if (Button == 0) {
				moduleData.state = BoardState::DISCHARGING;
				moduleData.swFuse = FuseState::OK;
				PowerSwitch = 0;      //on
				moduleData.outputOn = 1;
			}
		}
		if (moduleConfig.swFuseAutoResetEnable) { //auto reset
			if (HAL_GetTick() > (stopTime + TURN_ON_DELAY)) {
				moduleData.state = BoardState::DISCHARGING;
				moduleData.swFuse = FuseState::OK;
				PowerSwitch = 0;       //on
				moduleData.outputOn = 1;
			}
		}
	}

	//led signalization
	static int ledTime = 0;
	if (moduleConfig.ledEnable) {
		if (moduleData.swFuse != FuseState::OK) { //sw fuse trig
			Led = 1;
		}
		else {
			if (moduleData.outputOn) { //output on
				if (HAL_GetTick() > (ledTime + 500)) {
					Led = !Led;
					ledTime = HAL_GetTick();
				}
			}
			else Led = 0;
		}
	}
	else {
		Led = 0;
	}
}

//main function
int main(){
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3); //set low power mode
	
	ExtOn.SetPull(PinPull::UP); //initialize pins
	Button.SetPull(PinPull::UP);
	Led = 0;
	PowerSwitch = 1; //off
	moduleData.outputOn = 0;
	moduleData.state = BoardState::OUTPUT_OFF;
	
	if (Eeprom::IsConfigAvailable()) { //found config in EEPROM, load it
		Eeprom::LoadConfig(&moduleConfig);
	}
	else { //data not available, create default config
		InitDefaultConfig();
		Eeprom::SaveConfig(moduleConfig);
	}
	
	moduleData.swFuse = FuseState::OK;
	moduleData.hwFuse = FuseState::OK;  //fuses ok
	
	Monitor.SetBusVoltageRange(INA219::BusVoltageRange::RANGE_32V);
	Monitor.SetBusVoltageResolution(INA219::BusVoltageResolution::RES_12BIT);
	Monitor.SetShuntVoltageResolution(INA219::ShuntVoltageResolution::RES_12BIT_128S_69MS);
	Monitor.SetShuntVoltageGain(INA219::ShuntVoltageGain::GAIN_8_320MV);
	Monitor.SetSystemMode(INA219::SystemMode::MODE_POWERDOWN);
	
	SerialLink.StartRX();
	UpdateCellVoltages();
	UpdateDataINA();
	ProcessData();
	
	WakeupTimer.SetTimeoutAfter(0.5); //first measurement after 0.5sec
	while (1) {
		if (WakeupTimer.IsTimeoutDone()) {
			static int voltageLastRunTime = 0, inaRunLastTime = 0, runTime = 0;
			float time = std::min(moduleConfig.voltageSenseTime, moduleConfig.currentSenseTime);
			
			WakeupTimer.TimeoutDoneAck();
			WakeupTimer.SetTimeoutAfter(time);
			runTime += time * 1000;
			
			
			if (runTime > (voltageLastRunTime + moduleConfig.voltageSenseTime * 1000)) {
				voltageLastRunTime = runTime;
				UpdateCellVoltages();
				ProcessData();
			}
			if (runTime > (inaRunLastTime + moduleConfig.currentSenseTime * 1000)) {
				inaRunLastTime = runTime;
				UpdateDataINA();
				ProcessData();
			}
		}
		
		if (ExtOn) { //ExtOn not triggered
			__HAL_RCC_PWR_CLK_ENABLE();
			__HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);
			HAL_SuspendTick();
			HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI); //TODO: Use STOP mode instaed - IN FUTURE VERSION
			HAL_ResumeTick();
			WakeupTimer.EnableLSI();
		}
		
		SerialLink.HandleCommands(); //process commands
	}
}

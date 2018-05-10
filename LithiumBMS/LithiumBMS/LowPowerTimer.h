#ifndef LOW_POWER_TIMER_H_
#define LOW_POWER_TIMER_H_

#include "stm32l0xx_hal.h"

class LowPowerTimer {
public:
	LowPowerTimer();
	void EnableLSI();
	void SetTimeoutAfter(float seconds);
	void SetTimeoutDone();
	
	bool IsTimeoutDone() {
		return done_;
	}
	
	void TimeoutDoneAck() {
		done_ = 0;		
	}
	
	
	void LPTIM_ISR();
	
private:
	LPTIM_HandleTypeDef handle_;
	bool done_;
};


#endif

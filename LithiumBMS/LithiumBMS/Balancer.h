#ifndef BALANCER_H_
#define BALANCER_H_

#include "STM32_Cpp/GpioPin.h"

class Balancer{
public:
	Balancer();
	void SetAku(int id, bool state) {
		pins_[id] = state;
	}
	
	bool GetStatus(int id) {
		return pins_[id];
	}
	
private:
	GpioPin pins_[6];
};

#endif

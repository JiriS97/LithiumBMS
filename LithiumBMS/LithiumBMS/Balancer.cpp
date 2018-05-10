#include "Balancer.h"

Balancer::Balancer()
	: pins_{ PA12, PA11, PB5, PB4, PA8, PA7 }
{
	for (int i = 0; i < 6; i++)
	{
		pins_[i].Output();
		pins_[i] = 0;
	}	
}
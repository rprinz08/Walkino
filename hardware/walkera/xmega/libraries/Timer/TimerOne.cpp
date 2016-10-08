#include "TimerOne.h"

TimerOne Timer1;

unsigned short TimerOne::pwmPeriod = 0;
unsigned char TimerOne::clockSelectBits = 0;
void (*TimerOne::isrCallback)() = NULL;

ISR(TIMER1_OVF_vect)
{
  Timer1.isrCallback();
}

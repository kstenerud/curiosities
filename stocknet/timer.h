#ifndef _TIMER_H
#define _TIMER_H

#include <sys/time.h>

class Timer
{
public:
   Timer(time_t size): _pulse_size(size), _next_pulse(::time(NULL)+size)
                     , _set(0) {}

   // reset the timer.
   void   reset()              {_next_pulse = ::time(NULL) + _pulse_size;}

   // how much time is left.
   time_t left()               {_update(); return (_next_pulse - ::time(NULL));}

   // is the even set?
   int    isset()              {_update(); return _set;}

   // clear the event.
   void   clear()              {_set = 0;}
private:
   void   _update();		// update the timer
   time_t _pulse_size;		// time between event pulses
   time_t _next_pulse;		// next pulse time
   int    _set;			// event setting
};

#endif

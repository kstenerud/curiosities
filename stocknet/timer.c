#include "timer.h"

void Timer::_update()
{
   // get the current time.
   time_t current_time = ::time(NULL);

   // check for event.
   if(current_time >= _next_pulse)
   {
      _set = 1;
      // I'd rather have the timer miss a beat than to have it back up with
      // events
      if(current_time > _next_pulse + _pulse_size)
         _next_pulse = current_time + _pulse_size;
      else
         _next_pulse += _pulse_size;
   }
}

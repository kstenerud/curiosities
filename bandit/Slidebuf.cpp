#include "slidebuf.h"

//==============================================================================
// Name:   SlideBuffer::add
// Desc:   Add something to the sliding window buffer
// Proto:  int SlideBuffer::add(char* data, int seq_num)
// Args:   data    - pointer to the data to be added
//         seq_num - the sequence number of this data
// Return: TRUE/FALSE (Success/Failure)
bool SlideBuffer::add(char* data, int seq_num)
{
   unsigned long idx = seq_num%_size;		// index to data ptr array

   if(_num_in_buff <= 0)
   {
      _num_in_buff = 0;
      _current = seq_num;
   }

   // make sure it's valid
   if(seq_num > _max || seq_num < _current)
      return false;


   // over high water mark
   if(seq_num >= _current + _high)
//   if(_num_in_buff >= _high)
   {
   // If the buffer's been empty for awhile, chances are we've missed a bunch
   // of packets.  There's also a chance that the packets that will get through
   // from this point will be outside of the "current" window.  So here we
   // just set the window to something sane.
      _high_func();
      if(seq_num >= _current + _size)	// saves about 2 cycles to put it here
         return false;
   }

   if(_has_data[idx])
      return false;

   _data[idx] = data;			// store the data
   _has_data[idx] = true;		// flag that there's data here
   _num_in_buff++;			   // keep count
   return true;
}


//==============================================================================
// Name:   SlideBuffer::read
// Desc:   Get the next available buffer from the window
// Proto:  int SlideBuffer::next(char* data, int seq_num)
// Args:   void
// Return: pointer to the next available data or NULL.
char* SlideBuffer::next()
{
   unsigned long idx = _current%_size;		// index to data ptr array

   if(_num_in_buff <= 0)
   {
      _num_in_buff = 0;
      _low_func();
      return false;
   }

   _current++;
   if(_has_data[idx])				// found some data!
   {
      _has_data[idx] = 0;			// no data here anymore
      if(--_num_in_buff < _low)			// check for low water
         _low_func();
      return _data[idx];			// return the data
   }
   return NULL;					// no data in this entry.
}

void SlideBuffer::reset()
{
   _current = _num_in_buff = 0;
   memset(_has_data, 0, sizeof(char)*_size);
}


#ifndef _SLIDEBUF_H
#define _SLIDEBUF_H

#include <string.h>

//==============================================================================
// Name:  SlideBuffer
//
// Desc:  This is a sliding window buffer object with automatic sequencing, high
//        and low water mark handling.
//        All you do is add packets with a sequence number and SlideBuffer
//        handles the rest.  You can pretty much add whatever you want to it.
//        As long as you provide a sequence number, SlideBuffer dowsn't care
//        what you put in it.
//
// Proto: SlideBuffer(unsigned long size, unsigned long max,
//                                      void (*high_func)(), void (*low_func)())
//
// Args:  size      - the size of the window
//        max       - the maximum sequence number
//        high_func - the function to call when high water mark is reached
//        low_func  - the function to call when low water mark is reached
//
// Notes: SlideBuffer uses 32-bit unsigned integers internally, and does some
//        calculations whose result is larger than max. As such, you shouldn't
//        use sequence numbers over 31 bits or you could get unpredictable
//        results.  Most people don't need sequence numbers over 2 billion
//        anyway =)
//        ** This is a simplified version with no sequence number rollover.
//        Since we're using 4K packets and 2 billion sequence numbers,
//        It would take 2 years of straight playing for a sequence number
//        rollover to occur.  So naturally it's better to save the CPU cycles.
//==============================================================================
class SlideBuffer
{
public:
   SlideBuffer(unsigned long size, unsigned long max, void (*high_func)(),
                                                             void (*low_func)())
    : _size(size), _max(max), _high_func(high_func), _low_func(low_func)
    , _high((size*8)/10), _low((size*2)/10), _data(new char*[size])
    , _has_data(new char[size]), _current(0), _num_in_buff(0)
    {memset(_has_data, 0, sizeof(char)*_size);}
   ~SlideBuffer() {delete [] _data; delete [] _has_data;}

   bool  add(char* data, int seq_num);	// add something to the window
   char* next();			// read the next buffer in the window
   int  percent_full() {return (int)(((float)_num_in_buff / (float)_size)*100);}
   void reset();
   void set_high(int val) {_high = val;}
   void set_low(int val) {_low = val;}
   int num_in_buff() {return _num_in_buff;}
private:
   void            (*_high_func)();	// function to call at high water mark
   void            (*_low_func)();	// function to call at low water mark
   unsigned long   _size;		// size of buffer
   unsigned long   _max;		// max sequence number
   unsigned long   _low;		// low water mark
   unsigned long   _high;		// high water mark
   unsigned long   _num_in_buff;	// number of packets in buffer
   unsigned long   _current;		// current sequence number
   char**          _data;		// the pointer array itself
   char*           _has_data;		// flag if has data in it
};

#endif

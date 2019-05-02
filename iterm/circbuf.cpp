#include "circbuf.h"

#include <stdio.h>

//------------------------------------------------------------------------------
// Name:       CircularBuffer (constructor)
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       constructor
// Inputs:     num: size of the buffer in bytes
// Return:     -
// Notes:      -
CircularBuffer::CircularBuffer(int num)
: _top(new char[num]), _bottom(_top+num), _head(_top), _tail(_top)
{
   char str[40];

   // quick little hack to ensure a unique name for this event object
   sprintf(str, "data_ready_%x", this);
   _data_ready = CreateEvent(NULL, TRUE, FALSE, str);
}

//------------------------------------------------------------------------------
// Name:       ~CircularBuffer (destructor)
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       destructor
// Inputs:     -
// Return:     -
// Notes:      -
CircularBuffer::~CircularBuffer()
{
   delete [] _top;
   CloseHandle(_data_ready);
}

//------------------------------------------------------------------------------
// Name:       put
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       Put 1 char into the buffer.
// Inputs:     ch: the character to insert into the buffer
// Return:     TRUE (succes) / FALSE (buffer full)
// Notes:      -
int CircularBuffer::put(char ch)
{
	if( (_tail==_bottom && _head==_top) || (_tail+1 == _head) ) // full?
   {
      SetEvent(_data_ready);        // make sure event is set
		return FALSE;
   }
	*_tail++ = ch;                   // add the char to the buffer
	if(_tail > _bottom)              // circle around if at end of data space
		_tail = _top;
   if(_head != _tail)               // if data is in the buffer
      SetEvent(_data_ready);        // set the event

	return TRUE;
}

//------------------------------------------------------------------------------
// Name:       get
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       Get 1 char from the buffer.
// Inputs:     ch: the character space to be filled in
// Return:     TRUE (success) / FALSE (buffer empty)
// Notes:      -
int CircularBuffer::get(char* ch)
{
	if( _head == _tail)              // empty?
   {
      ResetEvent(_data_ready);      // make sure event is reset
		return FALSE;
   }
	*ch = *_head++;                  // take 1 char off the buffer
	if(_head > _bottom)              // circle around if at end of data space
		_head = _top;
	if( _head == _tail)              // no data left in buffer?
      ResetEvent(_data_ready);      // reset event

	return TRUE;
}

//------------------------------------------------------------------------------
// Name:       waitchar
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       Get 1 char from the buffer with timeout.
// Inputs:     ch: the character space to be filled in
//             timeout: time (in ms) to wait for a character to become available
// Return:     TRUE (success) / FALSE (timeout)
// Notes:      -
int CircularBuffer::waitchar(char* ch, DWORD timeout)
{
   // If buffer is empty, wait for the data to become available.
   // If it times out, return FALSE.
	if( _head == _tail)
      if (WaitForSingleObject(_data_ready, timeout) == WAIT_TIMEOUT)
		   return FALSE;

	*ch = *_head++;                  // take 1 char off the buffer
	if(_head > _bottom)              // circle around if at end of data space
		_head = _top;
	if( _head == _tail)              // no data left in buffer?
      ResetEvent(_data_ready);      // reset event

	return TRUE;
}

//------------------------------------------------------------------------------
// Name:       clear
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       Clear the circular buffer.
// Inputs:     -
// Return:     -
// Notes:      -
void CircularBuffer::clear()
{
   _head = _tail = _top;            // reset pointers
   ResetEvent(_data_ready);         // no data available
}


#ifndef _CIRCBUF_H
#define _CIRCBUF_H

#include <windows.h>

//------------------------------------------------------------------------------
// Name:       CircularBuffer
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:
//  This is an "event driven" circular buffer object.  It makes use of an event
//  object to allow the caller to wait a specified amount of time for data to
//  arrive in the buffer, and then timeout.
//
// ** See circbuf.cpp for description of methods

class CircularBuffer
{
public:
	CircularBuffer(int = 10000);     // constructor (default size 10000 bytes)
	~CircularBuffer();               // destructor
	int put(char);                   // put 1 char into the buffer
	int get(char*);                  // get 1 char from buffer
   int waitchar(char*, DWORD);      // get 1 char from buffer with timeout
   void clear();                    // clear the buffer
private:
	char* _top;                      // top of physical data space
	char* _bottom;                   // bottom of physical data space
	char* _head;                     // "head" of circular buffer
	char* _tail;                     // "tail" of circular buffer
   HANDLE _data_ready;              // signals when data is in the buffer
};
#endif

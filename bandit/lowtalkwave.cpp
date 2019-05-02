//lowtalkwave.cpp  --  This file contains the functions that deal with setting
//                       up the output device for recording (talking)
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
//FUNCTIONS:
//       bool TestOpenInputDevice(HWND);
//       void StartRecordTest(HWND);
//       void StopRecordTest(HWND);
//       void AllocTalkBuffers();
//       void CleanUpTalkBuffers();
#include <windows.h>
#include <mmsystem.h>
#include "intrsout.h"
#include "lowtalkwave.h"
#include "packet.h"
#include "slidebuf.h"

HWAVEIN  hwi;
WAVEHDR* talkBuffers[MAX_BUFFERS];

extern SlideBuffer* SBuf;
extern int packetSize;

char msg[MSG_LEN+1];

//==============================================================================
// name:   	TestOpenInputDevice
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	opens the input device to record data
// Inputs: 	hWnd -- used to output messages
// Output: 	BOOL -- whether or not opening the device was successful
// Notes:   Modified from WIN95 MULTIMEDIA & ODBC API BIBLE
bool TestOpenInputDevice(HWND hWnd)
{
   WAVEINCAPS   wic;
	WAVEFORMATEX wfx;
	UINT         nDevId;
	MMRESULT     rc;
	UINT         nMaxDevices = waveInGetNumDevs();

	hwi = NULL;

	for (nDevId = 0; nDevId < nMaxDevices; nDevId++)
	{
	   rc = waveInGetDevCaps(nDevId, &wic, sizeof(wic));
	   if (rc == MMSYSERR_NOERROR)
	   {
          wfx.nChannels = wic.wChannels;
          wfx.nSamplesPerSec = 22050;

	       wfx.wFormatTag      = WAVE_FORMAT_PCM;
       	 wfx.wBitsPerSample  = 8;
      	 wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
      	 wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
      	 wfx.cbSize          = 0;

          // open waveform input device
	       rc = waveInOpen(&hwi, nDevId, &wfx, (DWORD)(VOID*)waveInProc,
                                             (DWORD)hWnd, CALLBACK_FUNCTION);

	       if (rc == MMSYSERR_NOERROR)
	           break;
	       else
	       {
	           waveInGetErrorText(rc, msg, MSG_LEN),
	           MessageBox(hWnd, msg, NULL, MB_OK);
	           return(FALSE);
	       }
	   }
	}

   // device not found, error condition
	if (hwi == NULL)
	{
	    return(FALSE);
   }

   return(TRUE);
}

//==============================================================================
// name:   	StartRecordTest
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:    Add buffers to the input queue and starts the recording.
// Inputs: 	hWnd -- used to output messages
// Output: 	VOID
// Notes:   Modified from WIN95 MULTIMEDIA & ODBC API BIBLE
void StartRecordTest(HWND hWnd)
{
	MMRESULT rc;
   int      counter;
   MMTIME   mmtime;

	// prepare buffer blocks and add to input queue
	for (counter = 0; counter < MAX_BUFFERS; counter++)
	{
	   rc = waveInPrepareHeader(hwi, talkBuffers[counter], sizeof(WAVEHDR));

	   // add buffers to the input queue
	   if (rc == MMSYSERR_NOERROR)
	       rc = waveInAddBuffer(hwi, talkBuffers[counter], sizeof(WAVEHDR));

	   if (rc != MMSYSERR_NOERROR)
	   {
	       waveInGetErrorText(rc, msg, MSG_LEN),
	       MessageBox(hWnd, msg, NULL, MB_OK);
          StopRecordTest(hWnd);  // free allocated memory
	       return;
	   }
	}

	//start recording
	rc = waveInStart(hwi);

	//test waveInGetPosition() function
   mmtime.wType = TIME_SAMPLES;

   rc = waveInGetPosition(hwi, &mmtime, sizeof(MMTIME));

   if (rc != MMSYSERR_NOERROR)
	{
	    waveInGetErrorText(rc, msg, MSG_LEN),
	    MessageBox(hWnd, msg, NULL, MB_OK);
	}
}

//==============================================================================
// name:   	StopRecordTest
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	to stop recording and close the input device
// Inputs: 	hWnd -- used to output messages
// Output: 	VOID
// Notes:   Modified from WIN95 MULTIMEDIA & ODBC API BIBLE
void StopRecordTest(HWND hWnd)
{
   int counter;

   // stop recording
	waveInStop(hwi);
   waveInReset(hwi);

	// Unprepare headers
	for (counter = 0; counter < MAX_BUFFERS; counter++)
	  waveInUnprepareHeader(hwi, talkBuffers[counter], sizeof(WAVEHDR));

	waveInClose(hwi);

   CleanUpTalkBuffers();
}

//==============================================================================
// name:   	AllocTalkBuffers
// design:  Michael Gray
// code:    Michael Gray
// desc:   	to allocate the buffers that will be used to fill when talking by
//				the input device
// Inputs:  none
// Output: 	VOID
// Notes:
void AllocTalkBuffers()
{
   int counter;

   // allocate MAX_BUFFERS WAVEHDR buffer blocks
	for (counter = 0; counter < MAX_BUFFERS; counter++)
	{
	   talkBuffers[counter] = (wavehdr_tag*)HeapAlloc( GetProcessHeap(),
	                      HEAP_ZERO_MEMORY,
	                      sizeof(WAVEHDR) );
	   if (talkBuffers[counter])
	   {
	       talkBuffers[counter]->lpData = (char*)HeapAlloc( GetProcessHeap(),
	                                  HEAP_ZERO_MEMORY,
	                                  packetSize);
	       talkBuffers[counter]->dwBufferLength = packetSize;
	   }
	}
}

//==============================================================================
// name:   	CleanUpTalkBuffers
// design:  Michael Gray
// code:    Michael Gray
// desc:   	to free the buffer memory
// Inputs:  none
// Output: 	VOID
// Notes:
void CleanUpTalkBuffers()
{
   int counter;

   // free the WAVEHDR buffer blocks
 	for (counter = 0; counter < MAX_BUFFERS; counter++)
	{
     if (talkBuffers[counter] != NULL)
     {
	      HeapFree(GetProcessHeap(), 0, talkBuffers[counter]->lpData);
	      HeapFree(GetProcessHeap(), 0, talkBuffers[counter]);
         talkBuffers[counter] = NULL;
     }
	}
}

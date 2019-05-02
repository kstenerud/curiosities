//lowlistenwave.cpp  --  This file contains the functions that deal with setting
//                       up the output device for playing
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
//FUNCTIONS:
//       bool TestOpenOutputDevice(HWND hWnd, WAVEFORMATEX wfx);
//       void StartPlayBackTest(HWND hWnd);
//       void AddInitialBuffersToQueue(HWND hWnd);
//       void StopPlayBackTest(HWND hWnd);
//       void AllocListenBuffers();
//       void CleanUpListenBuffers();
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include "intrsout.h"
#include "lowlistenwave.h"
#include "packet.h"
#include "slidebuf.h"

HWAVEOUT hwo;
WAVEHDR* listenBuffers[MAX_BUFFERS];

char msg[MSG_LEN+1];

//originally declared in intrsout.cpp
extern int packetSize;
extern bool LISTEN;

//extern bool OUTPUTDEVICE_FREE;
extern SlideBuffer* SBuf;

//originally declared in initWin.cpp
extern HWND statusWnd;

//==============================================================================
// name:   	TestOpenOutputDevice
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	to open the output device
// Inputs: 	hWnd -- used to output messages
//          wfx -- format that the device must accomodate when open
// Output: 	BOOL -- whether or not opening the device was successful
// Notes:   Modified from WIN95 MULTIMEDIA & ODBC API BIBLE
bool TestOpenOutputDevice(HWND hWnd, WAVEFORMATEX wfx)
{
   WAVEOUTCAPS  woc;
	UINT         nDevId;
	MMRESULT     rc;
	UINT         nMaxDevices = waveOutGetNumDevs();

	hwo = NULL;

	for (nDevId = 0; nDevId < nMaxDevices; nDevId++)
	{
	   rc = waveOutGetDevCaps(nDevId, &woc, sizeof(woc));
	   if (rc == MMSYSERR_NOERROR)
	   {
	       rc = waveOutOpen(&hwo, nDevId, &wfx, (DWORD)hWnd, 0,
	                        CALLBACK_WINDOW);

	       if (rc == MMSYSERR_NOERROR)
          {
              DWORD dwVol;

              // set volume level to at least 80%
              rc = waveOutGetVolume( hwo, &dwVol );

              if (rc == MMSYSERR_NOERROR)
                  if (LOWORD(dwVol) < 0xCCCC ||
                     (wfx.nChannels == 2 && HIWORD(dwVol) < 0xCCCC))
                      rc = waveOutSetVolume(hwo,
                                             (DWORD)MAKELONG(0xCCCC, 0xCCCC));
          }

          if (rc != MMSYSERR_NOERROR)
	       {
	           waveOutGetErrorText(rc, msg, MSG_LEN),
	           MessageBox(hWnd, msg, NULL, MB_OK);
	           return(FALSE);
	       }
	       break;
	   }
	}

   // device not found, error condition
	if (hwo == NULL)
	    return(FALSE);

   return(TRUE);
}

//==============================================================================
// name:   	StartPlayBackTest
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	to start playing information received, but first some buffers are
//				added to the playback queue
// Inputs: 	hWnd -- used to output messages
// Output: 	VOID
// Notes:   Modified from WIN95 MULTIMEDIA & ODBC API BIBLE
void StartPlayBackTest(HWND hWnd)
{
	MMRESULT    rc;
   WAVEOUTCAPS woc;
   UINT        nDevId;
   DWORD       dwPitch;
   DWORD       dwRate;

   rc = waveOutGetID(hwo, &nDevId);
   rc = waveOutGetDevCaps(nDevId, &woc, sizeof(woc));

   // increase pitch and playback rate by a multiplier of two
   // if device supports this function
   if (woc.dwSupport & WAVECAPS_PLAYBACKRATE)
   {
       rc = waveOutGetPlaybackRate(hwo, &dwRate);
       rc = waveOutSetPlaybackRate(hwo, MAKELONG(HIWORD(dwRate)*2,
                                                   LOWORD(dwRate)) );
   }

   if (woc.dwSupport & WAVECAPS_PITCH)
   {
       rc = waveOutGetPitch(hwo, &dwPitch);
       rc = waveOutSetPitch(hwo, MAKELONG(HIWORD(dwPitch)*2, LOWORD(dwPitch)));
   }

   // pause the device until the first buffers have been
   // written to the device's queue
   waveOutPause(hwo);

}

//==============================================================================
// name:   	AddInitialBuffersToQueue
// design:  Michael Gray
// code:    Micheal Gray
// desc:   	to add the buffers to the playback queue for the first time
//				need to wait until information has been received before we start
//				playing the sound
// Inputs: 	hWnd -- used to output messages
// Output: 	void
// Notes:
void AddInitialBuffersToQueue(HWND hWnd)
{
	int counter;
	MMRESULT rc;

	for (counter = 0; counter < MAX_BUFFERS; counter++)
	{
      recv_buffer(listenBuffers[counter]->lpData, packetSize, 1);

      listenBuffers[counter]->dwBufferLength = packetSize;

	   //prepare the buffers
	   rc = waveOutPrepareHeader(hwo, listenBuffers[counter], sizeof(WAVEHDR));

	   // write buffers to the queue
	   if (rc == MMSYSERR_NOERROR)
	       rc = waveOutWrite(hwo, listenBuffers[counter], sizeof(WAVEHDR));

	   if (rc != MMSYSERR_NOERROR)
	   {
	       waveOutGetErrorText(rc, msg, MSG_LEN),
	       MessageBox(hWnd, msg, NULL, MB_OK);
          StopPlayBackTest(hWnd);  // free allocated memory
	       return;
	   }
	}
   waveOutRestart(hwo);
}

//==============================================================================
// name:   	StopPlayBackTest
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	to close the playback device, clean up buffers
// Inputs: 	hWnd -- used to output messages
// Output: 	VOID
// Notes:
void StopPlayBackTest(HWND hWnd)
{
   MMRESULT rc;
   int      counter;

	//Stop playback and return buffers to application
   rc = waveOutReset(hwo);
   if (rc != MMSYSERR_NOERROR)
   {
   	MessageBox(hWnd, "Error reseting speaker device!", "ERROR",
                                                MB_ICONSTOP | MB_OK);
   }

	// cleanup
	for (counter = 0; counter < MAX_BUFFERS; counter++)
	  waveOutUnprepareHeader(hwo, listenBuffers[counter], sizeof(WAVEHDR));

	rc = waveOutClose(hwo);
   if (rc == MMSYSERR_NOERROR)
   {
      wsprintf(msg, "Wave File:  Stopped");
		SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
		SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) "Voice:  None");
      LISTEN = false;
   }
   else
   {
   	MessageBox(hWnd, "Error closing speaker device!", "ERROR",
                                             MB_ICONSTOP | MB_OK);
   }

   CleanUpListenBuffers();
}
//==============================================================================
// name:   	AllocListenBuffers
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	to allocate the buffers that will be used to play while listening by
//				the output device.
// Inputs:  none
// Output: 	VOID
// Notes:
void AllocListenBuffers()
{
   int counter;

   // allocate two WAVEHDR buffer blocks
	for (counter = 0; counter < MAX_BUFFERS; counter++)
	{
	   listenBuffers[counter] = (wavehdr_tag*)HeapAlloc( GetProcessHeap(),
	                      HEAP_ZERO_MEMORY,
	                      sizeof(WAVEHDR) );
	   if (listenBuffers[counter])
	   {
	       listenBuffers[counter]->lpData = (char*)HeapAlloc( GetProcessHeap(),
	                                  HEAP_ZERO_MEMORY,
	                                  packetSize);
	       listenBuffers[counter]->dwBufferLength = packetSize;
	   }
	}
}
//==============================================================================
// name:   	CleanUpListenBuffers
// design:  Richard J. Simon, modified by Michael Gray
// code:    Richard J. Simon, modified by Michael Gray
// desc:   	to free the buffer memory
// Inputs:  none
// Output: 	VOID
// Notes:
void CleanUpListenBuffers()
{
   int counter;

   // free the WAVEHDR buffer blocks
 	for (counter = 0; counter < MAX_BUFFERS; counter++)
	{
     if (listenBuffers[counter] != NULL)
     {
	      HeapFree(GetProcessHeap(), 0, listenBuffers[counter]->lpData);
	      HeapFree(GetProcessHeap(), 0, listenBuffers[counter]);
         listenBuffers[counter] = NULL;
     }
	}
}

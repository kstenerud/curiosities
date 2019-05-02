//highwave.cpp  --  This file contains the high level functions that deal with
//                       playing a local wave file
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
//FUNCTIONS:
//       void PlayFile(HWND, char *);
//       void CloseDevice();
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include "highwave.h"
#include "intrsout.h"

UINT deviceID = 0;
char msg[MSG_LEN+1];

//originally declared in initwin.cpp
extern HWND statusWnd;

//originally declared in intrsout.cpp
extern bool LISTEN;
extern bool PLAYING_SONG;

//==============================================================================
// name:   	PlayFile
// design:  Michael Gray
// code:    Michael Gray
// desc:   	Starts playing the local wave file
// Inputs: 	hwnd -- used to output messages
//          filename -- local file to play
// Output: 	BOOL -- whether or not opening the device was successful
// Notes:   Modified from WIN95 MULTIMEDIA & ODBC API BIBLE
void PlayFile(HWND hwnd, char *filename)
{
	MCIERROR mcirc;
	MCI_OPEN_PARMS	open;
	MCI_OPEN_PARMS	play;

	//indicate that the output device is being used
   LISTEN = true;
   //set playing song to true in case the user tries to exit program while
   //playing a wave file
   PLAYING_SONG = true;

   //NOTE:  This function uses higher level function calls than the previous
   //functions.  Mainly because we do not need the level of control that is
   //required for streaming.
	//open waveform-audio device
	open.dwCallback = (DWORD)hwnd;
	open.lpstrDeviceType = "waveaudio";
	open.lpstrElementName = filename;

	mcirc = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,
               										(DWORD)&open);

	if (mcirc)
	{
		mciGetErrorString(mcirc, msg, MSG_LEN);
	    MessageBox(hwnd, msg, NULL, MB_ICONEXCLAMATION | MB_OK);
	}

	//play back the audio file
	deviceID = open.wDeviceID;

	play.dwCallback = (DWORD)hwnd;
	mcirc = mciSendCommand(deviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&play);
}
//==============================================================================
// name:   	CloseDevice
// design:  Michael Gray
// code:    Michael Gray
// desc:   	opens the input device to record data
// Inputs:  none
// Output:
// Notes:   This function is only used for local files because we are using high
//          level functions.
void CloseDevice()
{
	mciSendCommand(deviceID, MCI_CLOSE, 0, (DWORD)0);
   LISTEN = false;
   PLAYING_SONG = false;
   SendMessage(statusWnd, SB_SETTEXT, (WPARAM)1, (LPARAM)"Wave File:  Stopped");
}

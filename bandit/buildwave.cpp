//buildwave.cpp  --  This file contains the functions to save a wave file as it
//                   is being streamed.
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
#include <windows.h>
#include "intrsout.h"
#include "buildwave.h"

//global variables dealing with creating a file as it is received
HMMIO recFile;
char recFileName[256];
MMCKINFO recRiffChunk;
MMCKINFO recDataChunk;
MMCKINFO recFmtChunk;

char msg[MSG_LEN];

//originally declared in intrsout.cpp
extern int packetSize;

//originally declared in initwin.cpp
extern HWND localWnd;

//==============================================================================
// name:    CreateWave
// design:  Michael Gray
// code:    Michael Gray
// desc:    This function creates the file we will save the streaming info into.
// Inputs:  hWnd -- main window
//          wfx -- format that file must use
// Output:  none
// Notes:   This function mostly modeled after code in the Win95 Multimedia
//           and ODBC Bible
void CreateWave(HWND hWnd, WAVEFORMATEX wfx)
{
   long version;
   MMIOINFO mmioInfo;
   MMRESULT rc;

   //rename file if the client already has a filename of the one requested from
   //remote
   mmioRename(recFileName, "old.wav", NULL, 0);

   //open the wave file for creation and writing
   memset(&mmioInfo, 0, sizeof(MMIOINFO));
   mmioInfo.fccIOProc = mmioStringToFOURCC("WAV ", 0);
	recFile = mmioOpen(recFileName, &mmioInfo,
                        MMIO_CREATE | MMIO_WRITE | MMIO_ALLOCBUF);
   if (!recFile)
   {
   	MessageBox(hWnd, "Failed opening file for creation!", "ERROR", MB_OK);
      return;
   }

   version = mmioSendMessage(recFile, USR_MMIOM_PROC_VERSION, 0, 0);

   //create the main RIFF Chunk
   mmioSeek(recFile, 0, SEEK_SET);
   recRiffChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
   recRiffChunk.cksize = 0;
   rc = mmioCreateChunk(recFile, &recRiffChunk, MMIO_CREATERIFF);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}

   //create format chunk
   recFmtChunk.ckid = mmioStringToFOURCC("fmt ", 0);
   recFmtChunk.cksize = sizeof(WAVEFORMATEX);
   rc = mmioCreateChunk(recFile, &recFmtChunk, 0);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}
	rc = mmioWrite(recFile, (HPSTR)&wfx, sizeof(WAVEFORMATEX));
	if (rc == -1)
	{
	 	wsprintf(msg, "Error writing wfx to new file!");
   	MessageBox(hWnd, msg, "ERROR", MB_OK);
	}

	rc = mmioAscend(recFile, &recFmtChunk, 0);

   //create data chunk
   recDataChunk.ckid = mmioStringToFOURCC("data", 0);
	//allocate an arbitrary amount, it will be updated when we ascend out of the
   //data chunk once all of the file has be received and written
   recDataChunk.cksize = packetSize * 4000;

   rc = mmioCreateChunk(recFile, &recDataChunk, 0);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}

	//now the file has been set up for writing to, once all of the packets
   //have been streamed over and written we will call FinishCreateWave() to
   //clean up
}
//==============================================================================
// name:    FinishCreateWave
// design:  Michael Gray
// code:    Michael Gray
// desc:    Once all data has been written to the file this function will close
//          it.
// Inputs:  hWnd -- main window
// Output:  none
// Notes:   This function mostly modeled after code in the Win95 Multimedia
//           and ODBC Bible
void FinishCreateWave(HWND hWnd)
{
	MMRESULT rc;

   //ascend out of data chunk
	rc = mmioAscend(recFile, &recDataChunk, 0);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}

	//ascend out of main RIFF chunk
   rc = mmioAscend(recFile, &recRiffChunk, 0);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}

   rc = mmioFlush(recFile, 0);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}

   rc = mmioClose(recFile, 0);
	if (rc != MMSYSERR_NOERROR)
	{
	 	waveInGetErrorText(rc, msg, MSG_LEN),
   	MessageBox(hWnd, msg, NULL, MB_OK);
	}

   //now that we have a copy on client machine, add it to the local list
	SendMessage(localWnd, LB_ADDSTRING, 0, (LPARAM)recFileName);
}

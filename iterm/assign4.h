#ifndef ASSIGN4_H
#define ASSIGN4_H

/****************************************************************************


ASSIGN#.H


*****************************************************************************/

#define FONTX 7        // font width
#define FONTY 14       // font height
#define SCREENX 80     //80 screen width (in characters)
#define SCREENY 25     //25 screen height (in characters)
#define BUFFER_SIZE 1024 //circular buffer is 50 positions
#define WINDOWS95_COMMCONFIG_VERSION	0x100

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <process.h> //added
#include <string.h>
#include "circbuf.h"
#include "xsession.h"
#include "xdatalink.h"
#include "windisp.h"
//#include "vt100rx.h"
#include "resrcs.h"

#pragma warning (disable: 4096)
#pragma argsused

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam);
DWORD WINAPI EmuRead (LPDWORD lpdwParam1);
void locProcessorCommError(DWORD dwError);
int writeStringToPort(char* str);
int writeByteToPort(char ch);
int writeDataToPort(char* str, int len);
BOOL CALLBACK DialDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PortDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif




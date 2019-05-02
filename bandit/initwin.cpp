//initwin.cpp  --  This file contains functions that set up the child windows
//                   of the application.  These windows are essentially our GUI.
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
//FUNCTIONS:
//       void InitLocalList(HWND);
//       void InitRemoteList(HWND);
//       void InitStatusBar(HWND);
//       void InitTrackBar(HWND);
//       void InitProgressBar(HWND);
//       void InitErrorList(HWND);
//       void InitButtons(HWND);
//       LRESULT CALLBACK LocalListProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
//       LRESULT CALLBACK RemoteListProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
#include <windows.h>
#include <commctrl.h>
#include "initwin.h"
#include "intrsout.h"

//globals
HWND statusWnd;
HWND localWnd;
HWND remoteWnd;
HWND trackWnd;
HWND progressWnd;
HWND errorWnd;
HWND refreshLocal;
HWND refreshRemote;
HWND refreshBehaviour;
HWND stopLocal;
HWND pauseLocal;
HWND resumeLocal;
HWND pauseRemote;
HWND resumeRemote;
HWND stopRemote;
WNDPROC lList;
WNDPROC rList;

//originally declared in intrsout.cpp
extern HINSTANCE hInst;

//==============================================================================
// name:   	InitLocal List
// design:  Michael Gray
// code:    Michael Gray
// desc:   	Initializes the list box that will hold the local wave files
// Inputs: 	hwnd -- main window (parent)
// Output:
// Notes:
void InitLocalList(HWND hWnd)
{
   HDC hdc;
	TEXTMETRIC tm;

	hdc = GetDC(hWnd);

	SelectObject(hdc, GetStockObject (SYSTEM_FIXED_FONT));
   GetTextMetrics(hdc, &tm);
   ReleaseDC(hWnd, hdc);

   localWnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, "listbox",
         NULL, WS_CHILDWINDOW | WS_VISIBLE | LBS_STANDARD,
         20, 75,
         tm.tmAveCharWidth * 13 +
         GetSystemMetrics (SM_CXVSCROLL),
         tm.tmHeight * 5,
         hWnd, (HMENU) USR_PLAYLOCAL,
         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
         NULL);

   lList = (WNDPROC) SetWindowLong(localWnd, GWL_WNDPROC,
                                                    (LPARAM) LocalListProc) ;

	SendMessage(localWnd, LB_DIR, DDL_READWRITE, (LPARAM) "*.wav");
}
//==============================================================================
// name:   	InitRemoteList
// design:  Michael Gray
// code:    Michael Gray
// desc:   	Initializes the list box that will hold the remote wave files
// Inputs: 	hwnd -- main window (parent)
// Output:
// Notes:
void InitRemoteList(HWND hWnd)
{
   HDC hdc;
	TEXTMETRIC tm;

   hdc = GetDC(hWnd);

	SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));
   GetTextMetrics (hdc, &tm);
   ReleaseDC (hWnd, hdc);

	remoteWnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, "listbox",
         NULL, WS_CHILDWINDOW | WS_VISIBLE | LBS_STANDARD,
         250, 75,
         tm.tmAveCharWidth * 13 +
         GetSystemMetrics(SM_CXVSCROLL),
         tm.tmHeight * 5,
         hWnd, (HMENU) USR_PLAYREMOTE,
         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
         NULL);

	rList = (WNDPROC) SetWindowLong(remoteWnd, GWL_WNDPROC,
                                                   (LPARAM) RemoteListProc);
}
//==============================================================================
// name:   	InitStatusBar
// design:  Michael Gray
// code:    Michael Gray
// desc:   	Initializes the status bar
// Inputs: 	hwnd -- main window (parent)
// Output:
// Notes:
void InitStatusBar(HWND hWnd)
{
	RECT dimensions;
   int counter;
   int sections[3];

   GetClientRect(hWnd, &dimensions);

   for (counter = 1; counter <= 3; counter++)
   	sections[counter - 1] = dimensions.right/3 * counter;

   statusWnd = CreateWindow(STATUSCLASSNAME, "", WS_CHILD | WS_VISIBLE,
   	0, 0, 0, 0, hWnd, NULL, hInst, NULL);

   SendMessage(statusWnd, SB_SETPARTS, (WPARAM) 3, (LPARAM) sections);
   SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) "Voice:  None");
   SendMessage(statusWnd, SB_SETTEXT, (WPARAM)1, (LPARAM)"Wave File:  Stopped");
   SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 2, (LPARAM) "Connection:  None");
}
//==============================================================================
// name:   	LocalListProc
// design:  Michael Gray
// code:    Michael Gray
// desc:    Call back for local list box
// Inputs: 	Regular callback parameters
// Output:
// Notes:
LRESULT CALLBACK LocalListProc(HWND hWnd, UINT iMsg, WPARAM wParam,
                                                         LPARAM lParam)
{
	if (iMsg == WM_KEYDOWN && wParam == VK_RETURN)
   	SendMessage (GetParent(hWnd), WM_COMMAND, USR_PLAYLOCAL,
                                    MAKELONG(hWnd, LBN_DBLCLK));
	return CallWindowProc(lList, hWnd, iMsg, wParam, lParam) ;
}
//==============================================================================
// name:   	RemoteListProc
// design:  Michael Gray
// code:    Michael Gray
// desc:    Call back for remote list box
// Inputs: 	Regular callback parameters
// Output:
// Notes:
LRESULT CALLBACK RemoteListProc(HWND hWnd, UINT iMsg, WPARAM wParam,
                                                      LPARAM lParam)
{
	if (iMsg == WM_KEYDOWN && wParam == VK_RETURN)
   	SendMessage (GetParent(hWnd), WM_COMMAND, USR_PLAYREMOTE,
                                    MAKELONG(hWnd, LBN_DBLCLK));
	return CallWindowProc(rList, hWnd, iMsg, wParam, lParam);
}
//==============================================================================
// name:   	InitTrackBar
// design:  Michael Gray
// code:    Michael Gray
// desc:    Initializes the track bar
// Inputs: 	hWnd -- main window
// Output:
// Notes:   This GUI feature is not used at the moment
void InitTrackBar(HWND hWnd)
{
   HDC hdc;
	TEXTMETRIC tm;
   HWND trackText;

	hdc = GetDC(hWnd);

	SelectObject(hdc, GetStockObject (SYSTEM_FIXED_FONT));
   GetTextMetrics(hdc, &tm);
   ReleaseDC(hWnd, hdc);

	trackText = CreateWindow("static", "Remote File Playback",
   		WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT,
         210, (30 - (tm.tmHeight + 4)),
         tm.tmAveCharWidth * 30, (tm.tmHeight),
         hWnd, NULL,
         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
         NULL);

	trackWnd = CreateWindow(TRACKBAR_CLASS, "Trackbar",
						WS_CHILD | WS_VISIBLE | TBS_BOTTOM | WS_BORDER,
                  210, 30, 200, 30,
                  hWnd, NULL,
                  (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
                   NULL);

   SendMessage(trackWnd, TBM_SETPOS, (WPARAM) 1, (LPARAM) 0);
}
//==============================================================================
// name:   	InitProgressBar
// design:  Michael Gray
// code:    Michael Gray
// desc:    Initializes the track bar
// Inputs: 	hWnd -- main window
// Output:
// Notes:
void InitProgressBar(HWND hWnd)
{
   HDC hdc;
	TEXTMETRIC tm;
	HWND progressText;

   hdc = GetDC(hWnd);

	SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));
   GetTextMetrics (hdc, &tm);
   ReleaseDC (hWnd, hdc);

	progressText = CreateWindow("static", "Playback Delay",
   		WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT,
         20, (370 - (tm.tmHeight + 5)),
         tm.tmAveCharWidth * 15, (tm.tmHeight + 4),
         hWnd, NULL,
         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
         NULL);

	progressWnd = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "Progress Bar",
   						WS_CHILD | WS_VISIBLE | WS_BORDER,
                     20, 370, 300, 20,
                     hWnd, NULL,
	                  (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
   	                NULL);

   SendMessage(progressWnd, PBM_SETSTEP, (WPARAM) 1, 0);
}
//==============================================================================
// name:   	InitErrorList
// design:  Michael Gray
// code:    Michael Gray
// desc:    Initializes the list box that keeps track of application behaviour
// Inputs: 	hWnd -- main window
// Output:
// Notes:
void InitErrorList(HWND hWnd)
{
   HDC hdc;
	TEXTMETRIC tm;
   HWND errorText;

	hdc = GetDC(hWnd);

	SelectObject(hdc, GetStockObject (SYSTEM_FIXED_FONT));
   GetTextMetrics(hdc, &tm);
   ReleaseDC(hWnd, hdc);

	errorText = CreateWindow("static", "Application Behaviour",
   		WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT,
         20, (220 - (tm.tmHeight + 5)),
         tm.tmAveCharWidth * 20, (tm.tmHeight + 3),
         hWnd, NULL,
         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
         NULL);

   errorWnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, "listbox",
         NULL, WS_CHILDWINDOW | WS_VISIBLE | LBS_STANDARD,
         20, 220,
         tm.tmAveCharWidth * 45 +
         GetSystemMetrics (SM_CXVSCROLL),
         tm.tmHeight * 6,
         hWnd, (HMENU) USR_PLAYLOCAL,
         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
         NULL);

}
//==============================================================================
// name:   	InitButtons
// design:  Michael Gray
// code:    Michael Gray
// desc:    Initializes buttons on our GUI
// Inputs: 	hWnd -- main window
// Output:
// Notes:
void InitButtons(HWND hWnd)
{
   HDC hdc;
	TEXTMETRIC tm;
   HWND localGroup;
   HWND remoteGroup;

	hdc = GetDC(hWnd);

	SelectObject(hdc, GetStockObject (SYSTEM_FIXED_FONT));
   GetTextMetrics(hdc, &tm);
   ReleaseDC(hWnd, hdc);

	localGroup = CreateWindow("button", "Local Wave Files",
   						WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
							10, (75 - (tm.tmHeight + 5)),
                     210, 130,
                     hWnd, (HMENU) NULL,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	remoteGroup = CreateWindow("button", "Remote Wave Files",
   						WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
							240, (75 - (tm.tmHeight + 5)),
                     210, 130,
                     hWnd, (HMENU) NULL,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	refreshLocal = CreateWindow("button", "Refresh",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							20, (78 + tm.tmHeight * 5),
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_REFRESH_LOCAL,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	refreshRemote = CreateWindow("button", "Refresh",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							250, (78 + tm.tmHeight * 5),
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_REFRESH_REMOTE,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	refreshBehaviour = CreateWindow("button", "Refresh",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							20, (220 + tm.tmHeight * 6),
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_REFRESH_BEHAVIOUR,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	stopLocal = CreateWindow("button", "Stop",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							150, 75,
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_STOP_LOCAL,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	pauseLocal = CreateWindow("button", "Pause",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							150, 100,
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_PAUSE_LOCAL,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	resumeLocal = CreateWindow("button", "Resume",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							150, 125,
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_RESUME_LOCAL,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	stopRemote = CreateWindow("button", "Stop",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							380, 75,
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_STOP_REMOTE,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	pauseRemote = CreateWindow("button", "Pause",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							380, 100,
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_PAUSE_REMOTE,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);

	resumeRemote = CreateWindow("button", "Resume",
   						WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							380, 125,
                     tm.tmAveCharWidth * 8, tm.tmHeight + 6,
                     hWnd, (HMENU) USR_RESUME_REMOTE,
				         (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),
				         NULL);
}

//initwin.h
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
#ifndef	_INITWIN_H
#define	_INITWIN_H

void InitLocalList(HWND);
void InitRemoteList(HWND);
void InitStatusBar(HWND);
void InitTrackBar(HWND);
void InitProgressBar(HWND);
void InitErrorList(HWND);
void InitButtons(HWND);
LRESULT CALLBACK LocalListProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RemoteListProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

#endif

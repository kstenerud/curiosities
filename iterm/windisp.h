//------------------------------------------------------------------------------
//	WINDOW TEXT DISPLAY SYSTEM
// 
// NAME:        windisp.h
//
// SOURCE FILE: windisp.cpp
//
// FUNCTIONS: Handles all of the major functions required by most
//            terminal emulators eg. cursor movement, insert, delete, clear
//            screen etc.  See windisp.h for descriptions of functions.
//
//
// DATE: October 30 / 1996
//
// DESIGNER:   Karl Stenerud
// PROGRAMMER: Karl Stenerud
//
// NOTES:  The WindowDisplay object expects some preparatory work to be done
//         beforehand:
//         - The main window size should be fixed (no-resize) and not be allowed
//           to be maximized.
//         - The screen size must be set to a size that allows 80 characters
//           horizontally, and 25 chars vertically. (windisp is set to use the
//           "courier new" font)
//         - Note that some things such as menus and button bars take up pixels
//           from the client area of the window, and as such, you will have to
//           compensate when you determine the size of your window.
//
// Here is an example window definition if you are making a standard window with
// a menu:
/*
#include "windisp.h"

#define FONTX 7        // font width
#define FONTY 14       // font height
#define SCREENX 80     //80 screen width (in characters)
#define SCREENY 25     //25 screen height (in characters)

WindowDisplay* windowdisplay;
char szWinName[] = "IntelliTerm -- VT100 Emulator";
HWND hwnd;

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,
                    LPSTR lpszArgs, int nWinMode)
{
   ...

   if ((hwnd = CreateWindow(szWinName,
                       "IntelliTerm -- VT100 Emulator",
                       WS_OVERLAPPED | WS_CAPTION |
                       WS_SYSMENU | WS_MINIMIZEBOX |
                       DS_CENTER,               // define the window style
                       CW_USEDEFAULT,           // x-coordinate default
                       CW_USEDEFAULT,           // y-coordinate default
                       ((SCREENX+1)*FONTX),     // set the defined width + correction
                       ((SCREENY+2)*(FONTY+1)), // set the defined height + correction
                       HWND_DESKTOP,            // no parent window
                       NULL,                    // use menu registers with this class
                       hThisInst,               // handle of this instance of the program
                       NULL                     // no additional arguments
                       )) == NULL)
   	MessageBox(hwnd, "Unable to open main window.", NULL, MB_OK);

   windowdisplay = new WindowDisplay(hwnd, FONTX, FONTY, SCREENX, SCREENY);

   ...
}
*/
//------------------------------------------------------------------------------

#ifndef _WINDOW_DISPLAY_H
#define _WINDOW_DISPLAY_H
#include <windows.h>

class WindowDisplay
{
   private:
      int     _font_x;                              // monospaced font's width
      int     _font_y;                              // monospaced font's height
      int     _char_x;                              // current X position
      int     _char_y;                              // current Y position
      int     _max_char_x;                          // maximum X
      int     _max_char_y;                          // maximun Y
      int     _max_x;                               // maximum X (in pixels)
      int     _max_y;                               // maximun Y (in pixels)
      HFONT   _font_n;                              // normal font
      HFONT   _font_b;                              // bold font
      HWND    _hwnd;                                // current window handle
      HDC     _memdc;                               // common device context
      HBITMAP _hbit;                                // common bitmap handle

      void  PaintCursor();
      void  UnPaintCursor();
   public:
      void  SetTextFG(COLORREF rgb);
      void  ShiftUp(int count = 1);          // scroll up count lines
      void  WriteString(char* in);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::WindowDisplay (constructor)
// Func:   Presents a layered interface between the program and a text window
// Inputs: hwnd:  handle to an existing window
//         fontx: width (in pixels) of font being used
//         fonty: height (in pixels) of font being used
//         x:     display width (in characters)
//         y:     display height (on characters)
// Output: void
            WindowDisplay(HWND hwnd, int fontx, int fonty, int x, int y);
            ~WindowDisplay();
//------------------------------------------------------------------------------
// Name:   WindowDisplay::CursorUp
// Func:   Move the cursor up.
// Inputs: count: number of rows to move up
// Output: void
      void  CursorUp(int count = 1);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::CursorDown
// Func:   Move the cursor down.
// Inputs: count: number of rows to move down
// Output: void
      void  CursorDown(int count = 1);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::CursorRight
// Func:   Move the cursor right.
// Inputs: count: number of cols to move right
// Output: void
      void  CursorRight(int count = 1);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::CursorLeft
// Func:   Move the cursor left.
// Inputs: count: number of rows to move left
// Output: void
      void  CursorLeft(int count = 1);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::CursorReturn
// Func:   return cursor to col 0
// Inputs: void
// Output: void
      void  CursorReturn();

//------------------------------------------------------------------------------
// Name:   WindowDisplay::CursorXY
// Func:   set cursor X and Y position
// Inputs: x: X position (in characters)
//         x: Y position (in characters)
// Output: void
      void  CursorXY(int x, int y);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::HighlightOn
// Func:   turn on highlight (bold) text
// Inputs: void
// Output: void
      void  HighlightOn()  {SelectObject(_memdc, _font_b);}

//------------------------------------------------------------------------------
// Name:   WindowDisplay::HighlightOff
// Func:   turn off highlight (bold) text
// Inputs: void
// Output: void
      void  HighlightOff() {SelectObject(_memdc, _font_n);}

//------------------------------------------------------------------------------
// Name:   WindowDisplay::WriteChar
// Func:   write one character to the display
// Inputs: in: the character to be written to the display
// Output: void
      void  WriteChar(char in);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::Repaint
// Func:   Repaints the screen (useful after another window covered it)
// Inputs: void
// Output: void
      void  Repaint();

//------------------------------------------------------------------------------
// Name:   WindowDisplay::EraseChar
// Func:   Erases characters from the cursor position
// Inputs: count: number of characters to erase
// Output: void
      void  EraseChar(int count = 1);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::ClearLine
// Func:   Clears part or all of the line the cursor is currently on.  This
//         matches the VT100 arguments to ESC[xxxK
// Inputs: arg: 0: erase from the cursor to the end of the line
//              1: erase from the beginning of the line to here
//              2: erase the entire line
// Output: void
      void  ClearLine(int arg = 0);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::ClearScreen
// Func:   Clears part or all of the screen.  This matches the VT100 arguments
//         to ESC[xxxJ
// Inputs: arg: 0: erase from the line the cursor is on to the end of the screen
//              1: erase from the beginning of the screen to here
//              2: erase the entire screen
// Output: void
      void  ClearScreen(int arg = 0);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::DeleteChar
// Func:   Delete characters from where the cursor is, moving the rest of the
//         line left to replace it.
// Inputs: count: number of characters to delete
// Output: void
      void  DeleteChar(int count = 1);

//------------------------------------------------------------------------------
// Name:   WindowDisplay::InsertChar
// Func:   Insert blank spaces, moving existing text to the right to make room
// Inputs: count: number of spaces to insert
// Output: void
      void  InsertChar(int count = 1);
};

#endif
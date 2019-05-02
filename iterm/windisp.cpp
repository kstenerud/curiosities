//------------------------------------------------------------------------------
// NAME: windisp.cpp
//
// NOTES: See windisp.h for information
//------------------------------------------------------------------------------

#include "windisp.h"

WindowDisplay::WindowDisplay(HWND hwnd, int fontx, int fonty, int x, int y)
: _font_x(fontx), _font_y(fonty), _max_char_x(x), _max_char_y(y), _char_x(0)
, _char_y(0), _hwnd(hwnd), _max_x(x*fontx), _max_y(y*fonty)
, _font_n(CreateFont(fonty, fontx, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE, "Courier New"))
, _font_b(CreateFont(fonty, fontx, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE, "Courier New"))
{
          HDC     hdc = GetDC(_hwnd);               // get a device handle

   _memdc = CreateCompatibleDC(hdc);                // make a compatible handle
   _hbit = CreateCompatibleBitmap(hdc, _max_x, _max_y);
   SelectObject(_memdc, _hbit);                     // clear the window
   PatBlt(_memdc, 0, 0, _max_x, _max_y, WHITENESS); // by painting in white

   PaintCursor();                                   // put down the cursor
   WindowDisplay::HighlightOff();                   // start in normal font
   SetTextColor(_memdc, RGB(0, 0, 0));
   ReleaseDC(_hwnd, hdc);
}

WindowDisplay::~WindowDisplay()
{
   DeleteDC(_memdc);                                // destroy our objects
   DeleteDC(_font_n);
   DeleteDC(_font_b);
}

void WindowDisplay::CursorUp(int count)            // move crsr up
{
   UnPaintCursor();
   if((_char_y -= (count < 1 ? 1 : count)) < 0)    // make sure we move at least
                                                   // 1 character
//      _char_y = _max_char_y - 1;                   // wrap at top of screen
      _char_y = 0; // keep at top
   PaintCursor();
}

void WindowDisplay::CursorDown(int count)          // move crsr down
{
   UnPaintCursor();
   if((_char_y += (count < 1 ? 1 : count)) >= _max_char_y) // move at least 1
                                                           // character
//      _char_y = 0;                                 // wrap at bottom of screen
   {
      _char_y = _max_char_y - 1;
      ShiftUp();
   }

   PaintCursor();
}

void WindowDisplay::CursorLeft(int count)          // move crsr left
{
   UnPaintCursor();
   if((_char_x -= (count < 1 ? 1 : count)) < 0)    // move at least 1 char
   {
//      _char_x = _max_char_x - 1;                   // wrap at end and move up
//      CursorUp();
      _char_x = 0;
   }
   PaintCursor();
}

void WindowDisplay::CursorRight(int count)         // move crsr right
{
   UnPaintCursor();
   if((_char_x += (count < 1 ? 1 : count)) >= _max_char_x) // move at least 1
                                                           // character
   {
      _char_x = 0;                                 // wrap at end and move down
      CursorDown();
   }
   PaintCursor();
}

void WindowDisplay::CursorReturn()                 // Carriage Return
{
   UnPaintCursor();
   _char_x = 0;                                    // move to beginning of line
   PaintCursor();
}

void WindowDisplay::CursorXY(int x, int y)         // Move crsr to X, Y
{
   if(x < 0 || y < 0)                              // negative values are out
      return;
   UnPaintCursor();
   _char_x = (x <= _max_char_x ? x : 0);           // set X pos
   _char_y = (y <= _max_char_y ? y : 0);           // set Y pos
   PaintCursor();
}

void WindowDisplay::PaintCursor()                  // print a cursor
{
          RECT rect;                               // area to update after oper
   static HPEN black_pen = (HPEN) GetStockObject(BLACK_PEN);
          int  x_pos = _char_x * _font_x;          // top left corner
          int  y_pos = _char_y * _font_y;          // of rectangle

   rect.left   = (x_pos);                          // set position to update
   rect.right  = (x_pos + _font_x+1);
   rect.top    = (y_pos);
   rect.bottom = (y_pos + _font_y+1);

   SelectObject(_memdc, black_pen);                // use black pen
   MoveToEx(_memdc, x_pos, (y_pos + _font_y-1), NULL);
   LineTo(_memdc, (x_pos + _font_x), (y_pos + _font_y-1) ); // paint to screen

   InvalidateRect(_hwnd, &rect, FALSE);            // update this area

}

void WindowDisplay::UnPaintCursor()                // remove the cursor
{
          RECT rect;                               // area to update after oper
   static HPEN white_pen = (HPEN) GetStockObject(WHITE_PEN);
          int  x_pos = _char_x * _font_x;          // top left corner
          int  y_pos = _char_y * _font_y;          // of rectangle

   rect.left   = (x_pos);                          // set position to update
   rect.right  = (x_pos + _font_x+1);
   rect.top    = (y_pos);
   rect.bottom = (y_pos + _font_y+1);

   SelectObject(_memdc, white_pen);                // use white pen
   MoveToEx(_memdc, x_pos, (y_pos + _font_y-1), NULL);
   LineTo(_memdc, (x_pos + _font_x), (y_pos + _font_y-1) ); // write over crsr

   InvalidateRect(_hwnd, &rect, FALSE);            // update this area
}

void WindowDisplay::EraseChar(int count)           // erase characters
{
   RECT rect;                                      // area to update after oper
   int  x_pos = _char_x * _font_x;                 // top left corner
   int  y_pos = _char_y * _font_y;                 // of rectangle

   rect.left   = (x_pos);                          // set position to update
   rect.right  = (x_pos + (_font_x*count)+1);
   rect.top    = (y_pos);
   rect.bottom = (y_pos + _font_y+1);

   PatBlt(_memdc, (_char_x * _font_x)              // wipe out the character(s)
                , (_char_y * _font_y)
                , (_font_x*count)
                , (_font_y)
                , WHITENESS);
   InvalidateRect(_hwnd, &rect, FALSE);            // update display
}

void  WindowDisplay::ClearLine(int arg)            // clear line
{
   RECT rect;                                      // area to update after oper

   rect.left   = ( arg == 0 ? (_char_x * _font_x) : 0 );      // 0=start at crsr
   rect.right  = ( arg == 1 ? (_char_x * _font_x) : _max_x ); // 1=end at crsr
                                                              // 2=entire line
   rect.top    = (_char_y * _font_y);
   rect.bottom = (_char_y+1) * _font_y;
                                                   // wipe out the line
   PatBlt(_memdc, rect.left              , rect.top                 ,
                  (rect.right - rect.left), (rect.bottom - rect.top),
          WHITENESS);
   InvalidateRect(_hwnd, &rect, FALSE);            // update display
}

void  WindowDisplay::ClearScreen(int arg) // 0=cur-end, 1=beg-cur, 2=all
{
   RECT rect;                                      // area to update after oper

   rect.left   = 0;                                // erase across the whole
   rect.right  = _max_x;                           // screen.
   rect.top    = ( arg == 0 ? (_char_y * _font_y) : 0 );      // 0=start at crsr
   rect.bottom = ( arg == 1 ? (_char_y * _font_y) : _max_y ); // 1=end at crsr
                                                              // 2=entire screen
                                                   // wipe otu the screen
   PatBlt(_memdc, rect.left              , rect.top                 ,
						(rect.right - rect.left), (rect.bottom - rect.top),
          WHITENESS);

   PaintCursor();                                  // put the cursor back!
   InvalidateRect(_hwnd, &rect, FALSE);            // update display
}

void WindowDisplay::SetTextFG(COLORREF rgb)
{
            SetTextColor(_memdc, rgb);
}

void WindowDisplay::WriteString(char* in)
{
   int i = 0;
   int len = strlen(in);

   for(;i<len;i++)
      WriteChar(in[i]);
}

void WindowDisplay::WriteChar(char in)             // write a character
{
	char   str[2] = "";                      // holds char in string form

   switch(in)                                      // do ASCII emulation
   {
      case '\x07':                                 // ^G
         Beep(440, 1);
         break;
      case '\x08':                                 // backspace
         CursorLeft();
         break;
      case '\x09':                                 // horizontal tab
         CursorRight(8);
         break;
      case '\x0a':                                 // linefeed
         CursorDown();
         break;
      case '\x0c':                                 // form feed
         ClearScreen();
         break;
      case '\x0d':                                 // CR
         CursorReturn();
         break;
      default:
         if(in >= ' ' && in <= '~')                // otherwise output char
         {                                         // if it's printable
UnPaintCursor();
            SetBkMode(_memdc, TRANSPARENT);
            str[0] = in;                           // set up string

            PatBlt(_memdc, (_char_x * _font_x)     // erase old char
                         , (_char_y * _font_y)
                         , (_font_x)
                         , (_font_y)
                         , WHITENESS);

            TextOut(_memdc, (_char_x * _font_x)
                          , (_char_y * _font_y), str, 1); // put in new char

            WindowDisplay::CursorRight();          // move the cursor
PaintCursor();
         }
   }
}

void WindowDisplay::Repaint()                      // standard Windows repaint
{
   PAINTSTRUCT paintstruct;
   HDC hdc = BeginPaint(_hwnd, &paintstruct);

   BitBlt(hdc, 0, 0, _max_x, _max_y+_font_y, _memdc, 0, 0, SRCCOPY);
   EndPaint(_hwnd, &paintstruct);
}

void WindowDisplay::DeleteChar(int count)          // delete characters
{
   RECT rect;                                      // area to update after oper
   int start_x = _font_x * _char_x;                // top left corner
   int start_y = _font_y * _char_y;                // of rectangle
   int del_x   = _font_x * (count < 1 ? 1 : count); // # of pixels to delete

   if(del_x > _max_x - start_x)                    // not too many!
      return;

   rect.left   = start_x;                          // set up area to update
   rect.right  = _max_x;
   rect.top    = start_y;
   rect.bottom = start_y + _font_y;

   UnPaintCursor();
   SelectObject(_memdc, _hbit);

                                // delete the characters by moving dest over src
   BitBlt(_memdc, start_x                   , start_y, // dest
                  (_max_x - start_x - del_x), _font_y, // w, h
          _memdc, (start_x + del_x)         , start_y, // src
          SRCCOPY);
                                                   // white out the rest
   PatBlt(_memdc, (_max_x - del_x) , start_y, start_x, _font_y, WHITENESS);
   InvalidateRect(_hwnd, &rect, FALSE);            // update display
   PaintCursor();
}

void WindowDisplay::ShiftUp(int count)          // scroll up count lines
{
   int start_x = 0;
   int start_y = _font_y * (count < 1 ? 1 : count);
   int move_y   = _max_y - start_y; // # of pixels to move up

   if(move_y < 1)                    // not too many!
      return;

   UnPaintCursor();
   SelectObject(_memdc, _hbit);

   BitBlt(_memdc, 0      , 0,       // dest
                  _max_x , move_y,  // w, h
          _memdc, start_x, start_y, // src
          SRCCOPY);
                                                   // white out the rest
   PatBlt(_memdc, start_x , _max_y - start_y, _max_x, start_y, WHITENESS);
   InvalidateRect(_hwnd, NULL, FALSE);            // update display
   PaintCursor();
}

void WindowDisplay::InsertChar(int count)          // insert character places
{
   RECT rect;                                      // area to update after oper
   int start_x = _font_x * _char_x;                // top left corner
   int start_y = _font_y * _char_y;                // of rectangle
   int ins_x   = _font_x * (count < 1 ? 1 : count); // # of pixels to insert

   if(ins_x > _max_x - start_x)                    // not too many!
      return;

   rect.left   = start_x;                          // set up area to update
   rect.right  = _max_x;
   rect.top    = start_y;
   rect.bottom = start_y + _font_y;

   UnPaintCursor();
   SelectObject(_memdc, _hbit);
                                                   // move them over
   BitBlt(_memdc, (start_x + ins_x)         , start_y, // dest
                  (_max_x - start_x - ins_x), _font_y, // w, h
          _memdc, start_x                   , start_y, // src
          SRCCOPY);
                                                   // white out the new space
   PatBlt(_memdc, start_x, start_y, ins_x, _font_y, WHITENESS);
   InvalidateRect(_hwnd, &rect, FALSE);            // update display
   PaintCursor();
}

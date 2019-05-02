//------------------------------------------------------------------------------
// NAME: vt100rx.cpp
//
// NOTES: See vt100rx.h for information
//------------------------------------------------------------------------------

#include "vt100rx.h"
   
void VT100Rx(WindowDisplay* windisp, char value) // do some character emulation
{
   typedef enum {Norm, Ign, Esc, Csi} MODE;
// 4 modes of operation:
// Normal: all characters are sent as-is to the screen
// Ignore: Ignore an escape sequence.. just keep skipping until we reach
//         an alphabetic char (signalling end of escape sequence)
// Escape: Interpreting an escape sequnece
// CSI:    Interpreting a Control Sequence Initiator (VT100)

   static MODE mode    = Norm;            // start off in Normal mode
   static int  argno   = 0;               // space to store VT100 CSI arguments
   static int  arg[10] = {0,0,0,0,0,0,0,0,0,0}; // 10 because we are paranoid!

   switch(mode)
   {
      case Norm:                        // normal mode
         if(value == '\x1b')            // [ESC] sets us to Escape mode
            mode = Esc;
         else
            windisp->WriteChar(value);  // otherwise blast it to the screen
         break;
      case Ign:                         // Skip over a sequence we are ignoring
         if( (value >= 'a' && value <= 'z') ||
             (value <= 'A' && value <= 'z') )
            mode = Norm;
            break;
      case Esc:                         // Escape mode
         switch(value)
         {
            case 'c':                   // reset screen to defaults
               windisp->HighlightOff(); // all we can change is highlight
               break;
            case '[':
               mode = Csi;
               break;
            case '=':
            case '?':                   // we ignore these sequences
               mode = Ign;
               for(argno=0;argno<10;argno++) // clear arguments
                  arg[argno] = 0;
               argno=0;
            default:                    // Anything else is not an Esc sequence
               mode = Norm;
               break;
         }
         break;
      case Csi:                         // Control Sequence Initiator mode
         if(value >= '0' && value <= '9')  // decode an argument
            arg[argno] = (arg[argno]*10) + (int)value - '0';
         else if (value == ';')
            argno++;                    // move to next argument, if any
         else
         {
            switch(value)
            {
               case '@':                // insert char
                  windisp->InsertChar(arg[0]);
                  break;
               case 'A':                // crsr up
                  windisp->CursorUp(arg[0]);
                  break;
               case 'B':                // crsr down
                  windisp->CursorDown(arg[0]);
                  break;
               case 'C':                // crsr right
                  windisp->CursorRight(arg[0]);
                  break;
               case 'D':                // crsr left
                  windisp->CursorLeft(arg[0]);
                  break;
               case 'E':               // next line
                  windisp->CursorDown(arg[0]);
                  break;
               case 'F':               // prec line
                  windisp->CursorUp(arg[0]);
                  break;
               case 'H':               // set pos
                  windisp->CursorXY(arg[1],arg[0]);
                  break;
               case 'J':             // erase scrn (0=cur-end, 1=beg-cur, 2=all)
                  windisp->ClearScreen(arg[0]);
                  break;
               case 'K':             // erase line (0=cur-end, 1=beg-cur, 2=all)
                  windisp->ClearLine(arg[0]);
                  break;
               case 'P':               // delete char
                  windisp->EraseChar(arg[0]);
                  break;
               case 'm':               // text mode
                  for(int i = 0;i<=argno;i++)
                     switch(arg[i])
                     {
                        case 0:        // All we even consider is highlight
                        case '0':
                        case '2':
                           windisp->HighlightOff();
                           break;
                        case 1:
                           windisp->HighlightOn();
                           break;
                        default:
                           break;
                     }
            }
            for(argno=0;argno<10;argno++)  // clear arguments
               arg[argno] = 0;
            argno=0;
            mode = Norm;
         }
         break;
      default:                       // if all else fails, return to normal mode
         mode = Norm;
         break;
   }
}
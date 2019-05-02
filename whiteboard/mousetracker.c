//-----------------------------------------------------------------------------
//FUNCTION:	
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//
//RETURNS:
//DEPENDENCIES: wprintf.c used to display the position of the mouse. 
//NOTES:        displays the x and y coordinates of the mouse pointer in
//              a specified widget.  The display is cleared when the mouse
//              pointer leaves the widget.
//-----------------------------------------------------------------------------
#include <Xm/Xm.h>

#include "mousetracker.h"
#include "wprintf.h"
              
void TrackMousePosition ( Widget     w, 
                          XtPointer  clientData, 
                          XEvent    *event, 
                          Boolean   *flag ) 
{
   Widget tracker = (Widget)clientData;
   /*
    * Extract the position of the pointer from the event
    * and display it in the tracker widget. 
    */

    wprintf (  tracker, "X: %04d, Y: %04d", 
              event->xmotion.x, event->xmotion.y );
}

void ClearTracker( Widget    w,
                   XtPointer clientData,
                   XEvent    *event ,
                   Boolean   *flag )
{
    Widget tracker = (Widget)clientData;
 
    wprintf(tracker, " ", 0, 0);
}

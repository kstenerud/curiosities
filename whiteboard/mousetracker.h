/* mousetracker.h: Header file for mousetracker.c */

#include <Xm/Xm.h>

void TrackMousePosition(Widget w, XtPointer clientData, 
                        XEvent *event, Boolean *flag);

void ClearTracker(Widget w, XtPointer clientData, 
                  XEvent *event, Boolean *flag);



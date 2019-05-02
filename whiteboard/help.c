
/********************************************************************
 *         This example code is from the book:
 * 
 *           The X Window System: Programming and Applications with Xt
 *           Second OSF/Motif Edition
 *         by
 *           Douglas Young
 *           Prentice Hall, 1994
 *
 *         Copyright 1994 by Prentice Hall
 *         All Rights Reserved
 *
 *  Permission to use, copy, modify, and distribute this software for 
 *  any purpose except publication and without fee is hereby granted, provided 
 *  that the above copyright notice appear in all copies of the software.
 *
 *  Revisions:
 *     April 1997 - Added a freehand and text function to the DrawingCommands.
 *
 ********************************************************************/

/********************************************************
 * help.c: A utility callback function that supports
 *         context-sensitive help
 ********************************************************/
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <X11/cursorfont.h>

static char *GetHelpText ( Widget w );

void ContextHelpCallback ( Widget    w,
                           XtPointer clientData,
                           XtPointer callData )
{
   XmAnyCallbackStruct *cbs = ( XmAnyCallbackStruct * ) callData;
   Widget               selectedWidget = NULL;
   char                *helpString;
   Widget               dialog;
   static Cursor        cursor = (int)NULL;
   XEvent               event;
   Widget               parent = XtParent ( w ); /* In case w is a gadget */
   Widget               mainWindow = ( Widget ) clientData;

  /*
   * If called from an XmNhelpCallback (involked with <F1>)
   * the reason will be XmCR_HELP.  Extract the smallest
   * window in which the help request occurred, and find
   * the corresponding widget.
   */

   if ( cbs->reason == XmCR_HELP )
   {
      if ( cbs->event && cbs->event->xany.window )
         selectedWidget = XtWindowToWidget ( XtDisplay (parent ),
                                             cbs->event->xany.window );
   }
   else
   {
     /*
      * If the reason is not XmCR_HELP, this callback must have been
      * invoked from somewhere else, probably a help menu item.
      * Allow the user to interactively selevt a widget on which
      * help is required.
      */

      if ( !cursor )
         cursor = XCreateFontCursor ( XtDisplay ( parent ),
                                      XC_question_arrow );
      selectedWidget = XmTrackingEvent ( mainWindow, cursor,
                                         FALSE, &event );
   }

  /*
   * Check for a valid widget
   */

   if ( !selectedWidget )
      return;

  /*
   * Retrieve the text of a help message from the resource
   * database to the selected widget.
   */

   helpString = GetHelpText ( selectedWidget );

  /*
   * If some help text has been found, create a dialog to
   * display the information.
   */

   if ( helpString )
   {
      dialog = XmCreateInformationDialog ( mainWindow, "helpdialog",
                                           NULL, 0 );

      XtUnmanageChild ( XmMessageBoxGetChild ( dialog,
                                               XmDIALOG_HELP_BUTTON ) );
      XtUnmanageChild ( XmMessageBoxGetChild ( dialog,
                                               XmDIALOG_CANCEL_BUTTON ) );

      XtVaSetValues ( dialog,
                      XtVaTypedArg, XmNmessageString, XmRString,
                      helpString, strlen ( helpString ) + 1,
                      NULL );

      XtManageChild ( dialog );
   }
}

static char *GetHelpText ( Widget w )
{
   static char *returnValue = NULL;
   XtResource   requestResources;

  /*
   * Request the value of a "text" resource in the "help"
   * subpart of the given widget.
   */

   requestResources.resource_name   = "text";
   requestResources.resource_class  = "Text";
   requestResources.resource_type   = XmRString;
   requestResources.resource_size   = sizeof ( char * );
   requestResources.default_type    = XmRImmediate;
   requestResources.resource_offset = 0;
   requestResources.default_addr    = ( XtPointer ) NULL;

   XtGetSubresources ( w, ( XtPointer ) &returnValue,
                       "help", "Help",
                       &requestResources, 1, NULL, 0 );

   return ( returnValue );
}


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
 *     April 1997 - Added saving and loading of objects.
 *     April 1997 - Added network support for editor interface.
 ********************************************************************/
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/DrawingA.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include <Xm/Label.h>
#include <Xm/RepType.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include "draw.h"
#include "connectdialog.h"
#include "addtextdialog.h"
#include "mousetracker.h"
#include "MenuSupport.h"
#include "socket.h"

#define DISCONNECTMESSAGE "Do you really want to disconnect?"

/* Callbacks used by menu items. */
extern void wprintf(Widget , char *, int, int);

static void QuitCallback ( Widget    w, 
                           XtPointer clientData, 
                           XtPointer callData );
static void SaveCallback ( Widget    w, 
                           XtPointer clientData, 
                           XtPointer callData );
static void LoadCallback ( Widget    w, 
                           XtPointer clientData, 
                           XtPointer callData );

static void SetFigureCallback ( Widget    w, 
                                XtPointer clientData, 
                                XtPointer callData );
static void SetColorCallback ( Widget    w, 
                               XtPointer clientData, 
                               XtPointer callData );
static void ClearCallback ( Widget    w,
                            XtPointer clientData,
                            XtPointer callData );
static void ConnectCallback ( Widget    w,
                              XtPointer clientData,
                              XtPointer callData );
static void DisconnectCallback ( Widget    w,
                                 XtPointer clientData,
                                 XtPointer callData );
static void ReallyDisconnectCallback ( Widget w,
				       XtPointer clientData,
				       XtPointer callData);
static void CancelDialogCallback( Widget w, 
				  XtPointer clientData,
				  XtPointer callData);

/* Declare external functions called from this module */
extern void XmRepTypeInstallTearOffModelConverter ( void );
extern void SetDrawingFunction ( int type );
extern void SetCurrentColor ( Pixel pixel );
extern void LoadData ( void );
extern void SaveData ( void );
extern InitGraphics ( Widget w );
extern void ClearWindow(Widget w);
 
/* Descriptions of contents of the menu bar. See Chapter 6 */
static MenuDescription drawingCommandsDesc[] = {
  { (MenuType)TOGGLE,   "Move",       SetFigureCallback, ( XtPointer ) NONE },
  { (MenuType)TOGGLE,   "Line",       SetFigureCallback, ( XtPointer ) LINE   },
  { (MenuType)TOGGLE,   "Circle",     SetFigureCallback, ( XtPointer ) CIRCLE },
  { (MenuType)TOGGLE,   "Rectangle",   
                            SetFigureCallback, ( XtPointer ) RECTANGLE },
  { (MenuType)TOGGLE,   "FilledRectangle",
                            SetFigureCallback, ( XtPointer ) FILLEDRECTANGLE },
  { (MenuType)TOGGLE,   "FilledCircle",
                            SetFigureCallback, ( XtPointer ) FILLEDCIRCLE },
  { (MenuType)TOGGLE,   "FreeHand",   SetFigureCallback, ( XtPointer ) FREEHAND },
  { (MenuType)TOGGLE,   "Text",       SetFigureCallback, ( XtPointer ) SOMETEXT  },
  { (MenuType)TOGGLE,  "Eraser",      SetFigureCallback, ( XtPointer ) ERASE },
  {(MenuType)NULL}
                        
};

static MenuDescription colorDesc[] = {
  { TOGGLE,   "Black",   SetColorCallback },
  { TOGGLE,   "White",   SetColorCallback },
  { TOGGLE,   "Red",     SetColorCallback },
  { TOGGLE,   "Green",   SetColorCallback },
  { TOGGLE,   "Blue",    SetColorCallback },
  { TOGGLE,   "Magenta", SetColorCallback },
  { TOGGLE,   "Cyan",    SetColorCallback },
  { (MenuType)NULL}
};

static MenuDescription filePaneDesc[] = {
  { BUTTON,   "Save",            SaveCallback },
  { BUTTON,   "Load",            LoadCallback },
  { SEPARATOR                                 },
  { BUTTON,   "Quit",            QuitCallback },
  { (MenuType) NULL}
};

static MenuDescription optionsDesc[] = {
  { BUTTON,   "Clear Window",    ClearCallback },
  { BUTTON,   "Connect",         ConnectCallback },
  { BUTTON,   "Disconnect",      DisconnectCallback },
  { (MenuType) NULL}
};

static MenuDescription helpPaneDesc[] = {
  { BUTTON,   "HelpOnContext",   ContextHelpCallback },
  { (MenuType)NULL}
};

static MenuDescription menuBarDesc[] = {
  { PULLDOWN,        "File",   NULL, NULL, filePaneDesc },
  { PULLDOWN,        "Options",
                               NULL, NULL, optionsDesc },
  { RADIOPULLDOWN,   "Colors", NULL, NULL, colorDesc    },
  { RADIOPULLDOWN,   "DrawingCommands",
                               NULL, NULL, drawingCommandsDesc },
  { HELPPANE,        "Help",   NULL, NULL, helpPaneDesc },
  { (MenuType) NULL}
};

//-----------------------------------------------------------------------------
//FUNCTION:	CreateDrawingEditor()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:	CreateDrawingEditor(Widget parent)
//		 
//              -parent: the parent widget to the drawing editor.
//RETURNS:	Widget
//DEPENDENCIES: depends on wprintf.c used for displaying the mouse position
//              information in the editor. 
//PRECONDITION: wprintf is defined.
//NOTES:        creates the main GUI for the drawing program.  Sets up the
//              options for tear-off menus, registers callbacks, menubars,
//              and creates a tracker widget used for displaying mouse
//              position information.
//-----------------------------------------------------------------------------
                    
Widget CreateDrawingEditor ( Widget parent )
{
    Widget canvas, mainWindow, tracker;

   /*
    * Install the tear-off type converter to allow
    * the tear-off model to be set in a resource file.
    */
    XmRepTypeInstallTearOffModelConverter();

   /*
    * Create a main window widget to handle the layout
    * for the drawing editor.
    */
    mainWindow = XtCreateManagedWidget ( "mainWindow", 
                                         xmMainWindowWidgetClass, 
                                         parent, NULL, 0 );
   /*
    * Add a menu bar to the window. Pass the main window widget
    * as the default client, needed primarily by the help callback.
    */
    CreateMenu ( MENUBAR, "menuBar", mainWindow,
                 menuBarDesc, ( XtPointer ) mainWindow );

   /*
    * Create the widget used as a drawing surface 
    */
    canvas = XtCreateManagedWidget ( "canvas",
                                     xmDrawingAreaWidgetClass, 
                                     mainWindow, NULL, 0 );
   
   /* Create the tracker widget and register event handlers for the
    * target widget.
    */
    tracker = XtCreateManagedWidget("mousetracker",
                                    xmLabelWidgetClass, 
                                    mainWindow, NULL, 0);

    wprintf(tracker, " ", 0, 0);
    XtAddEventHandler(canvas, LeaveWindowMask, FALSE, ClearTracker,
                      (XtPointer)tracker);
    XtAddEventHandler(canvas, PointerMotionMask, FALSE, TrackMousePosition,
                      (XtPointer)tracker);
   
    /* Initialize the display list and other related functions. */
    XtVaSetValues(mainWindow, XmNworkWindow, canvas,
                             XmNmessageWindow, tracker,
                             NULL); 

    InitGraphics ( canvas );
    
    return mainWindow;
}

//-----------------------------------------------------------------------------
//FUNCTION:	QuitCallback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    QuitCallback(Widget w,
//                           XtPointer clientData,
//                           XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: socket.c for disconnecting a current connection.
//PRECONDITION: none
//NOTES:        Executed when the user 'quits' the application.
//-----------------------------------------------------------------------------
static void QuitCallback ( Widget    w, 
                           XtPointer clientData,
                           XtPointer callData )
{
     SockManager* smanager = SockManager::Instance();
     smanager->disconnect();
     
     exit ( 0 );
}

//-----------------------------------------------------------------------------
//FUNCTION:	SaveCallback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    SaveCallback(Widget w,
//                           XtPointer clientData,
//                           XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: calls SaveData()
//PRECONDITION: none
//NOTES:        Executed when the user 'saves' a drawing session
//-----------------------------------------------------------------------------
static void SaveCallback ( Widget    w, 
                           XtPointer clientData,
                           XtPointer callData )
{
    SaveData();
}

//-----------------------------------------------------------------------------
//FUNCTION:	LoadCallback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    LoadCallback(Widget w,
//                           XtPointer clientData,
//                           XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: calls LoadData()
//PRECONDITION: none
//NOTES:        Executed when the user 'loads' a previously saved session.
//              Does not currently broadcast across the network.
//-----------------------------------------------------------------------------
static void LoadCallback ( Widget    w,
                           XtPointer clientData,
                           XtPointer callData )
{
    LoadData();
}

//-----------------------------------------------------------------------------
//FUNCTION:	ConnectCallback()
//DATE:		May 6, 1997
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    ConnectCallback(Widget w,
//                              XtPointer clientData,
//                              XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: connectdialog.c for creating a dialog box for connection.
//PRECONDITION: none
//NOTES:        Executed when the user selects to connect to a server.
//-----------------------------------------------------------------------------
static void ConnectCallback ( Widget    w,
                              XtPointer clientData,
                              XtPointer callData )
{
    static Widget connectDialog = NULL;

       connectDialog = CreateDialog(w, "connectDialog", ConnectOKCallback,
                                                        ConnectCancelCallback);
       
       XtVaSetValues(connectDialog,
                     XmNdialogStyle,
                     XmDIALOG_FULL_APPLICATION_MODAL,
                     NULL);
       
       XtManageChild(connectDialog);
}

//-----------------------------------------------------------------------------
//FUNCTION:	DisconnectCallback()
//DATE:		May 6, 1997
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    DisconnectCallback(Widget w,
//                                XtPointer clientData,
//                                XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:        Executed when the user wants to disconnect from the server.
//-----------------------------------------------------------------------------
static void DisconnectCallback ( Widget    w,
                                 XtPointer clientData,
                                 XtPointer callData )
{
    static Widget disconnectDialog = NULL;
    

    if(!disconnectDialog)
    {
      disconnectDialog = XmCreateQuestionDialog(w,"reallyDisconnectDialog",
						NULL,0);

      XtUnmanageChild(XmMessageBoxGetChild(disconnectDialog,
                      XmDIALOG_HELP_BUTTON));

      XtVaSetValues(disconnectDialog,
		    XmNdialogStyle,
		    XmDIALOG_FULL_APPLICATION_MODAL,
		    XtVaTypedArg, XmNmessageString, XmRString,
		    DISCONNECTMESSAGE, strlen(DISCONNECTMESSAGE)+1,
		    NULL);
	
      XtAddCallback(disconnectDialog, XmNokCallback,
		    ReallyDisconnectCallback, (XtPointer) NULL);
      XtAddCallback(disconnectDialog, XmNcancelCallback,
		    CancelDialogCallback, (XtPointer) NULL);
    }
    XtManageChild(disconnectDialog);
    
}

//-----------------------------------------------------------------------------
//FUNCTION:	SetColorCallback()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    SetColorCallback(Widget w,
//                               XtPointer clientData,
//                               XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: calls SetCurrentColor()
//PRECONDITION: none
//NOTES:        Executed when a color is chosen from the 'Colors' pull-down menu.
//-----------------------------------------------------------------------------
static void SetColorCallback ( Widget    w,
                               XtPointer clientData,
                               XtPointer callData )
{
    Widget     widget = XtParent ( w );
    Display   *dpy  = XtDisplay ( w );
    int        scr    = DefaultScreen ( dpy );
    Colormap   cmap   = DefaultColormap ( dpy, scr );
    XColor     color, ignore;
    
    XmToggleButtonCallbackStruct *cbs =
                         ( XmToggleButtonCallbackStruct * ) callData;

   /* 
    * Allocate the color named by the name of the widget that
    * invoked this callback and call SetCurrentColor to install
    * the resulting pixel and the current color.
    */

    if ( cbs->set &&
         XAllocNamedColor ( dpy, cmap, XtName ( w ), 
                            &color, &ignore ) )
        SetCurrentColor ( color.pixel );
}

//-----------------------------------------------------------------------------
//FUNCTION:     SetFigureCallback()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    ConnectCallback(Widget w,
//                              XtPointer clientData,
//                              XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: socket.c for disconnecting a current connection.
//              calls SetDrawingFunction()
//PRECONDITION: none
//NOTES:        Used when the user 'quits' the application.
//-----------------------------------------------------------------------------
static void SetFigureCallback ( Widget    w, 
                                XtPointer clientData,
                                XtPointer callData )
{
    XmToggleButtonCallbackStruct *cbs =
                         ( XmToggleButtonCallbackStruct * ) callData;
   /*
    * Set the current command from the parameter passed as clientData
    */

    if ( cbs->set )
        SetDrawingFunction ( (unsigned int) clientData );
}

//-----------------------------------------------------------------------------
//FUNCTION:	ClearCallback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    ClearCallback(Widget w,
//                              XtPointer clientData,
//                              XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: calls ClearWindow()
//PRECONDITION: none
//NOTES:        Used when the user 'quits' the application.
//-----------------------------------------------------------------------------
static void ClearCallback ( Widget    w,
                            XtPointer clientData,
                            XtPointer callData )
{
    ClearWindow(w);    
}

//-----------------------------------------------------------------------------
//FUNCTION:	ReallyDisconnectCallback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    ReallyDisconnectCallback(Widget w,
//                                       XtPointer clientData,
//                                       XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: socket.c for disconnecting a current connection.
//PRECONDITION: none
//NOTES:        Used when the user 'quits' the application.
//-----------------------------------------------------------------------------
static void ReallyDisconnectCallback(Widget w, XtPointer clientData,
				     XtPointer callData)
{
  SockManager* smanager = SockManager::Instance();
  smanager->disconnect();
   
}

//-----------------------------------------------------------------------------
//FUNCTION:	CancelDialogCallback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    CancelDialogCallback(Widget w,
//                                   XtPointer clientData,
//                                   XtPointer calldata)	
//
//RETURNS:	void
//DEPENDENCIES: nond
//PRECONDITION: none
//NOTES:        Executed when the user presses 'cancel'.
//-----------------------------------------------------------------------------
static void CancelDialogCallback(Widget w, XtPointer clientData,
				 XtPointer callData)
{
    XtUnmanageChild (w);
}

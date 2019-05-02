//-----------------------------------------------------------------------------
//FILE:	        connectdialog.c
//DATE:		May 6, 97
//REVISION:     
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//NOTES:        This file contains the necessary functions to create a dialog
//              box when the user tries to connect to the server or someone
//              who is already connected to a server in which case is
//              redirected to connect to the 'real' server automatcally.
//              The dialog box prompts the user for an ip address.  Port
//              number is specified in baerg.h
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/MessageB.h>
#include "connectdialog.h"
#include "baerg.h"
#include "socket.h"

//-----------------------------------------------------------------------------
//FUNCTION:     CreateDialog()	
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	CreateDialog(Widget parent, char *name,
//                           XtCallbackProc ConnectOkCallback,
//                           XtCallbackProc ConnectCancelCallback)
//		 -parent: parent widget of the dialog box.
//               -name: pointer to char for the name of the dialog box.
//RETURNS:	Widget
//DEPENDENCIES: DialogWidget type must be defined. (See connectdialog.h)
//PRECONDITION: none
//NOTES:        Creates a dialog box when the user chooses to CONNECT from
//              the Options pulldown menu.
//-----------------------------------------------------------------------------

Widget CreateDialog(Widget parent, char *name,
                    XtCallbackProc ConnectOkCallback,
                    XtCallbackProc ConnectCancelCallback)
{
   Widget dialog, rc;
   int i;
   DialogWidgets *widgets;

   widgets = (DialogWidgets *)XtMalloc(sizeof(DialogWidgets));

   /* Create the standard message dialog. */
   dialog = XmCreateMessageDialog(parent, name, NULL, 0);

   /* Remove unneeded children */
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_SYMBOL_LABEL));
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_MESSAGE_LABEL));
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));

   /* Add callbacks. */
   if(ConnectOkCallback)
      XtAddCallback(dialog, XmNokCallback,
                    ConnectOkCallback, widgets);
   if(ConnectCancelCallback)
      XtAddCallback(dialog, XmNcancelCallback,
                    ConnectCancelCallback, widgets);

   rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, dialog,
                                XmNnumColumns,  2,
                                XmNpacking,     XmPACK_COLUMN,
                                XmNorientation, XmVERTICAL,
                                NULL);

   /* Create the labels */
   XtCreateManagedWidget("IP Address", xmLabelWidgetClass, rc, NULL, 0);

   /* Create the text input field */
   widgets->ipAddress = XtCreateManagedWidget("IP Address",
                                              xmTextFieldWidgetClass,
                                              rc, NULL, 0);
   return (dialog);
}

/* This callback is used when 'OK' is selected in the dialog box */
void ConnectOKCallback(Widget w, XtPointer clientData, XtPointer callData)
{
   DialogWidgets *dialog = (DialogWidgets *)clientData;  
   char Address[80];

   SockManager* smanager = SockManager::Instance();	   
   strcpy(Address, XmTextFieldGetString(dialog->ipAddress));

   smanager->connect(Address, B_PORT); 
   
   XtUnmanageChild(w);
}

/* This callback is used when 'CANCEL' is selected in the dialog box */
void ConnectCancelCallback(Widget w,
                    XtPointer clientData,
                    XtPointer callData)
{
   XtUnmanageChild(w);
}

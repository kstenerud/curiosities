//-----------------------------------------------------------------------------
//FILE:	       addtextdialog.c
//DATE:	       May 6, 97
//REVISION:     
//DESIGNER:    Chris Torrens and Cam Mitchner
//PROGRAMMER:  Chris Torrens and Cam Mitchner
//NOTES:       This file contains the necessary functions to create a dialog box
//             for the 'text' drawing function.  The dialog box prompts the user
//             for the text to be displayed after clicking mouse button 1 on a
//             specific location on the drawing canvas.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/MessageB.h>
#include "addtextdialog.h"
#include "graphics.h"

//-----------------------------------------------------------------------------
//FUNCTION:	CreateTextDialog()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	CreateTextDialog(Widget parent, char *name, char *message,
//                               XtCallbackProc TextOkCallback,
//                               XtCallbackProc TextCancelCallback)
//               -parent: parent widget of the dialog box.
//		 -name: pointer to char of name of the dialog box.
//               -message: pointer to char of message to display in dialog box.
//RETURNS:      Widget	
//DEPENDENCIES: TextDialogWidgets type must be defined. (See addtextdialog.h) 
//PRECONDITION: none
//NOTES:        Used when the user chooses the TEXT type from the
//              DrawingCommands pulldown menu.
//---------------------------------------------------------------------------
Widget CreateTextDialog(Widget parent, char *name, char *message,
                    XtCallbackProc TextOkCallback,
                    XtCallbackProc TextCancelCallback)
{
   Widget dialog, rc;
   int i;
   TextDialogWidgets *widgets;

   widgets = (TextDialogWidgets *)XtMalloc(sizeof(TextDialogWidgets));

   /* Create the standard message dialog. */
   dialog = XmCreateMessageDialog(parent, name, NULL, 0);

   /* Remove unneeded children */
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_SYMBOL_LABEL));
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_MESSAGE_LABEL));
   XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));

   /* Add callbacks. */
   if(TextOkCallback)
      XtAddCallback(dialog, XmNokCallback,
                    TextOkCallback, widgets);
   if(TextCancelCallback)
      XtAddCallback(dialog, XmNcancelCallback,
                    TextCancelCallback, widgets);

   rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, dialog,
                                XmNnumColumns,  2,
                                XmNpacking,     XmPACK_COLUMN,
                                XmNorientation, XmVERTICAL,
                                NULL);
   /* Create the labels. */
   XtCreateManagedWidget(message, xmLabelWidgetClass, rc, NULL, 0);

   /* Create the text input field. */
   widgets->text = XtCreateManagedWidget("InputText",
                                              xmTextFieldWidgetClass,
                                              rc, NULL, 0);
   return (dialog);
}

/* This callback is used when 'OK' is selected in the text dialog box. */
void TextOkCallback(Widget w, XtPointer clientData, XtPointer callData)
{
   TextDialogWidgets *dialog = (TextDialogWidgets *)clientData;  

   setText(XmTextFieldGetString(dialog->text));

   XtUnmanageChild(w);
}

/* This callback is used when 'CANCEL' is selected in the text dialog box */
void TextCancelCallback(Widget w,
                    XtPointer clientData,
                    XtPointer callData)
{
   XtUnmanageChild(w);
}

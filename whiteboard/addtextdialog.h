/* addtextdialog.h: Header file for addtextdialog.c */

typedef struct {
   Widget text;
} TextDialogWidgets;

void TextOkCallback(Widget w, XtPointer clientdata, XtPointer calldata);
void TextCancelCallback(Widget w, XtPointer clientdata, XtPointer calldata);

Widget CreateTextDialog(Widget parent, char *name, char *text,
                    XtCallbackProc TextOkCallback,
                    XtCallbackProc TextCancelCallback);


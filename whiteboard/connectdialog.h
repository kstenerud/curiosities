/* connectdialog.h: Header file for connectdialog.c */

typedef struct {
   Widget ipAddress, port;
} DialogWidgets;

void ConnectOKCallback(Widget w, XtPointer clientdata, XtPointer calldata);
void ConnectCancelCallback(Widget w, XtPointer clientdata, XtPointer calldata);

Widget CreateDialog(Widget parent, char *name,
                    XtCallbackProc ConnectOkCallback,
                    XtCallbackProc ConnectCancelCallback);



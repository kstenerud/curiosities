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
 ******************************************************************************/

/*******************************************************
  * wprintf.c: A convenient function that displays text
  *            in a label or button, providing the same
  *            syntax as printf()
  *******************************************************/
#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <wprintf.h>

void wprintf ( Widget w, char *format, int x, int y ) 
{
    char     tempstr[80];
    XmString  xmstr;
    
    sprintf ( tempstr, format, x, y);
    xmstr =  XmStringCreateLtoR ( tempstr, XmFONTLIST_DEFAULT_TAG );
    XtVaSetValues ( w, XmNlabelString, xmstr, NULL );
    XmStringFree ( xmstr );
}

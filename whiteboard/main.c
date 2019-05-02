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
 ********************************************************************/

/*
** main.c - entry point for drawing program
*/
#include <Xm/Xm.h>
#include <stdio.h>
#include "socket.h"

extern Widget CreateDrawingEditor ( Widget parent );
extern void draw_callback(unsigned char id, unsigned char operation, char* data, int len);
extern void error_callback(char* error, char* desc);

void main ( int argc, char **argv )
{
    Widget       shell;
    XtAppContext app;

   /*
    * Initialize Xt, creating an application shell.
    * Call an external function to create the drawing 
    * editor as a child of the shell.
    */

    shell = XtAppInitialize ( &app, "Draw", NULL, 0, 
                              &argc, argv, NULL, NULL, 0 );

    
    SockManager::Init(B_PORT, app,draw_callback, error_callback,\
                 B_MULTI_MAX_CONNECT);

    CreateDrawingEditor ( shell );
    
    XtRealizeWidget ( shell );
    XtAppMainLoop ( app );
}
                    

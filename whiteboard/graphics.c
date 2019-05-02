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
 ****************************************************************************/

/**********************************************************
 * graphics.c: A display list that holds graphics objects
 *             and associated functions needed to
 *             display objects in a drawing area widget
 **********************************************************/
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include <stdlib.h>
#include "graphics.h"
#include "addtextdialog.h"
#include "parser.h"
#include "socket.h"
                                           
static short xPos;
static short yPos;
static char textObject[80];

GraphicsObject displayList[MAXOBJECTS];
TextObject     displayTextList[MAXTEXT];
FreeLineObject displayFreeLineList[MAXLINES];
XPoint         displayPointList[MAXPOINTS];

//-----------------------------------------------------------------------------
//FUNCTION:	InitGraphics()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:	InitGraphics(Widget w)
//               -w: widget that will contain the graphics objects.		 
// 
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:        makes the Widget w available to other functions by setting
//              the global variable 'canvas' to 'w'.  Callbacks added to 
//              handle screen refreshing and rubberbanding.
//-----------------------------------------------------------------------------


void InitGraphics ( Widget w )
{
   XGCValues values;

   /* 
   ** Remember the given widget for use by other functions.
   ** Retrieve the display and colormap for later use.
   **/
   canvas  = w; 
   display = XtDisplay ( canvas );
   XtVaGetValues(canvas, XmNcolormap, &colormap, NULL );


   /*
   ** Register a callback to redraw the display list when the
   ** window is exposed, and event handlers to handle adding
   ** new objects to the display list interactively
   **/
   XtAddCallback(canvas, XmNexposeCallback, Redisplay, NULL );
   XtAddEventHandler(canvas, ButtonPressMask, FALSE,
                     StartRubberBand, NULL );
   XtAddEventHandler(canvas, ButtonMotionMask, FALSE,
                     TrackRubberBand, NULL );
   XtAddEventHandler(canvas, ButtonReleaseMask, FALSE,
                     EndRubberBand, NULL );

   /* Get the colors of the canvas. */
   XtVaGetValues ( canvas, 
                    XmNforeground, &currentForeground,
                    XmNbackground, &background,
                    NULL );

   /* Fill in the values structure */
   values.foreground = currentForeground ^ background;
   values.function   = GXxor;

   /* Set the rubber band gc to use XOR mode. */
   xorGC = XtGetGC(canvas, GCForeground | GCFunction, &values);
}

//-----------------------------------------------------------------------------
//FUNCTION:	DRAWINGFUNCTIONS
//DATE:		1994
//REVISION:	Chris Torrens & Cam Mitchner
//               May 6, 1997 - Added Freehand and Text
//                 
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:	DRAWINGFUNCTION(Window w, GC gc, int x, int y, int x2, int y2)	 
//               - DRAWINGFUNCTION will be replaced by one of the items in the
//                 list specified under NOTES.
//RETURNS:	void
//DEPENDENCIES: defined drawing function calls in X.
//PRECONDITION: display variable is set.
//NOTES:        Handles the drawing of each object type.  This function header
//              applies to the following list of functions:
//               - DrawLine()
//               - DrawRectangle()
//               - DrawFilledRectangle()
//               - DrawCircle()
//               - DrawFilledCircle()
//               - DrawFreeHand()
//               - NewText()
//               - Erase()
//-----------------------------------------------------------------------------
void DrawLine(Window w, GC gc, int x, int y, int x2, int y2)
{
   XDrawLine(display, w, gc, x, y, x2, y2);
}

void DrawRectangle(Window w, GC gc, int x, int y, int x2, int y2 )
{
   FixDataOrdering(&x, &y, &x2, &y2);
   XDrawRectangle(display, w , gc,  x, y, x2 - x, y2 - y);
}

void DrawFilledRectangle(Window w, GC gc, int x, int y, int x2, int y2)
{
   FixDataOrdering(&x, &y, &x2, &y2);
   XFillRectangle(display, w, gc, x, y, x2 - x, y2 - y);
}

static void DrawCircle(Window w, GC gc, int x, int y, int x2, int y2)
{
   FixDataOrdering(&x, &y, &x2, &y2);
   XDrawArc(display,  w, gc, x, y, x2 - x, y2 - y, 0, 64 * 360);
}

static void DrawFilledCircle(Window w, GC gc, int x, int y, int x2, int y2)
{
   FixDataOrdering(&x, &y, &x2, &y2);
   XFillArc(display, w, gc, x, y, x2 - x, y2 - y, 0, 64 * 360);
}

static void DrawFreeHand(Window w, GC gc, int x, int y, int x2, int y2)
{
}

static void NewText(Window w, GC gc, int x, int y, int x2, int y2)
{
}

static void Erase(Window w, GC gc, int x, int y, int x2, int y2)
{
   XGCValues values;

   FixDataOrdering ( &x, &y, &x2, &y2 );

   /* Get a GC for the current colors. */

   values.foreground = background;
   values.background = background;

   gc = XtGetGC(canvas, GCForeground | GCBackground, &values);

   XFillRectangle(display, w, gc, x, y, x2 - x, y2 - y);
}

//-----------------------------------------------------------------------------
//FUNCTION:	FixDataOrdering()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:	FixDataOrdering(int *x, int *y, int *x2, int *y2)		 
// 
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:        Swaps the given points so that x2 is greater than x and
//              y2 is greater than y, if needed.
//-----------------------------------------------------------------------------
void FixDataOrdering(int *x, int *y, int *x2, int *y2)
{
   /*
    * Swap the given points so that x2 is greater than x 
    * and y2 is greater than y.
    */

   if(*x2 < *x)
   {
      int tmp = *x;
      *x      = *x2;
      *x2     = tmp;
   }
    
   if (*y2 < *y)
   { 
      int tmp = *y; 
      *y      = *y2; 
      *y2     = tmp;
   }
}

//-----------------------------------------------------------------------------
//FUNCTION:     error_callback()	
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	void error_callback(char *error, char *desc);
//		 
//              -error: pointer to char containing error type.
//              -desc:  pointer to char containing error description.
//RETURNS:      void	
//DEPENDENCIES: none 
//PRECONDITION: none 
//NOTES:        Used to display an error and a description of the error 
//              using a dialog box. 
//-----------------------------------------------------------------------------
void error_callback(char *error, char *desc)
{
   Widget dialog,rc;

   dialog = XmCreateMessageDialog(canvas, error, NULL, 0);

   XtUnmanageChild(XmMessageBoxGetChild(dialog,XmDIALOG_SYMBOL_LABEL));
   XtUnmanageChild(XmMessageBoxGetChild(dialog,XmDIALOG_MESSAGE_LABEL));
   XtUnmanageChild(XmMessageBoxGetChild(dialog,XmDIALOG_HELP_BUTTON));
   XtUnmanageChild(XmMessageBoxGetChild(dialog,XmDIALOG_CANCEL_BUTTON));

   XtAddCallback(dialog,XmNokCallback, ErrorOkCallback, NULL);

   rc = XtVaCreateManagedWidget("rc",xmRowColumnWidgetClass,dialog,
                                 XmNnumColumns, 1,
                                 XmNpacking, XmPACK_COLUMN,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

   XtCreateManagedWidget(desc,xmLabelWidgetClass,rc,NULL,0);
   XtManageChild(dialog);

}

void ErrorOkCallback(Widget w, XtPointer clientData, XtPointer callData)
{
   XtUnmanageChild(w);
}

//-----------------------------------------------------------------------------
//FUNCTION:	draw_callback()
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:    draw_callback(unsigned char id, unsigned char operation,
//                 char *data, int len)	
//	        -id:        id of the client (assigned by server);
//              -operation: operation to be broadcasted specified in baerg.h 
//RETURNS:      void
//DEPENDENCIES: 
//PRECONDITION: 
//NOTES:        a color is assigned to each client based on its id.
//              the figuretype   
//-----------------------------------------------------------------------------
void draw_callback(unsigned char id, unsigned char operation, char* data,int len)
{
   XColor color;
   XGCValues values;
   int figureType;

   switch(id)
   {
      case 0:
         color.red   = 0;
         color.green = 0;
         color.blue  = 0;
         break;

      case 1:
         color.red   = 65535;
         color.green = 65535;
         color.blue  = 65535;
         break;

      case 2:
         color.red   = 65535;
         color.green = 0;
         color.blue  = 0;
         break;

      case 3:
         color.red   = 0;
         color.green = 65535;
         color.blue  = 0;
         break;

      case 4:
         color.red   = 0;
         color.green = 0;
         color.blue  = 65535;
         break;

      case 5:
         color.red   = 0;
         color.green = 65535;
         color.blue  = 65535;
         break;
   }



   switch(operation)
   {
      case B_OP_POINTS:
         figureType = FREEHAND;
         break;

      case B_OP_LINE:
         figureType = LINE;
         break;

      case B_OP_RECT:
         figureType = RECTANGLE;
         break;

      case B_OP_ELLIPSE:
         figureType = CIRCLE;
         break;

      case B_OP_TEXT:
         figureType = SOMETEXT;
         break;

      case B_OP_FILL_RECT:
         figureType = FILLEDRECTANGLE;
         break;

      case B_OP_FILL_ELLIPSE:
         figureType = FILLEDCIRCLE;
         break;

      case B_OP_ERASE:
         figureType = ERASE;
         break;

      case B_OP_CLEARSCREEN:
         ClearWindow(canvas);
         break;
   }
   commandParser(display, canvas, data, len, operation, color, figureType);
}

//-----------------------------------------------------------------------------
//FUNCTION:	TypeToFunction()
//DATE:		1994
//REVISION:	May 6, 1997 - Chris Torrens & Cam Mitchner
//               - added FREELINE and TEXT functionality
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    DrawingFunction TypeToFunction(int type)	
//	        -type: integer of object type. (see draw.h)
//               
//RETURNS:      pointer to a function.
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        returns a pointer to a function that can draw the given type
//              of figure. 
//-----------------------------------------------------------------------------
static DrawingFunction TypeToFunction(int type)
{
   switch ( type )
   {
      case NONE:
         return NULL;

      case LINE:
         return DrawLine;

      case CIRCLE:
         return DrawCircle;

      case RECTANGLE:
         return DrawRectangle;

      case FILLEDCIRCLE:
         return DrawFilledCircle;

      case FILLEDRECTANGLE:
         return DrawFilledRectangle;

      case FREEHAND:
         return DrawFreeHand;

      case SOMETEXT:
         return NewText;

      case ERASE:
         return Erase;
    }
}

//-----------------------------------------------------------------------------
//FUNCTION:	SetDrawingFunction()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    SetDrawingFunction(int type)	
//	        -type: integer of object type. (see draw.h)
//               
//RETURNS:      void
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        Set both the current type and the current function 
//-----------------------------------------------------------------------------
void SetDrawingFunction(int type)
{
   currentFigureType = type;
   currentFunction = TypeToFunction(type);
}

//-----------------------------------------------------------------------------
//FUNCTION:     SetCurrentColor()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    SetCurrentColor(Pixel fg)	
//
//RETURNS:      void
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        Set the current foreground color
//-----------------------------------------------------------------------------
void SetCurrentColor(Pixel fg)
{
    currentForeground = fg;
}

//-----------------------------------------------------------------------------
//FUNCTION:	Redisplay()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    Redisplay(Widget w, XtPointer clientData, XtPointer callData)
//
//RETURNS:      void
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        Executed when the widget is 'exposed'.  Handles like a refresh.
//-----------------------------------------------------------------------------
static void Redisplay(Widget    w, 
                      XtPointer clientData,
                      XtPointer callData )
{
   int i;

   /* Loop through and draw all objects in the display list in sequence */
   for(i = 0; i < nextSlot; i++)
      (*(displayList[i].func))(XtWindow(w), 
                 displayList[i].gc,
                 displayList[i].x1,
                 displayList[i].y1,
                 displayList[i].x2,
                 displayList[i].y2);

   for( i = 0; i < numFreeLines; i++ )
      XDrawLines(display, XtWindow (w), 
                 displayFreeLineList[i].gc,
                 displayFreeLineList[i].points,
                 displayFreeLineList[i].num_points,
                 CoordModeOrigin);

   for(i = 0; i < numTextObjects; i++)
   {
      XDrawString(display, XtWindow(w),
                 displayTextList[i].gc,
                 displayTextList[i].x1,
                 displayTextList[i].y1,
                 displayTextList[i].str_buf,
                 displayTextList[i].str_size);
   }
}

//-----------------------------------------------------------------------------
//FUNCTION:	ClearWindow()
//DATE:		1994
//REVISION:	May 6, 1997 - Chris Torrens & Cam Mitchner
//               - Add FREELINE and TEXT handling capability
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    void ClearWindow(Widget w)
//
//RETURNS:      void
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        Called when the user chooses Clear Window from the Options menu
//-----------------------------------------------------------------------------
void ClearWindow(Widget w)
{
   int i;
   char data[8],otherdata[8];

   nextSlot = 0;

   for(i = 0; i < numFreeLines; i++)
      free(displayFreeLineList[i].points);

   numTextObjects = 0;   
   numFreeLines = 0;
   XClearArea(display, XtWindow(canvas), 0, 0, 0, 0, TRUE);
}

//-----------------------------------------------------------------------------
//FUNCTION:	PickObject()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    GraphicsObject *PickObject(int x, int y)
//          
//RETURNS:      void
//DEPENDENCIES: displayList[]
//PRECONDITION: none 
//NOTES:        Called when the user has chosen 'move' from the DrawingCommands
//              pull-down menu and and pointed and selected an object on the
//              drawing canvas.  Currently, no network support is functionable. 
//              Search the display list for an object that encloses the given
//              point.  Loop backwards so that the objects on top are found
//              first.  Currently, FREELINES and TEXT are not supported.
//-----------------------------------------------------------------------------
GraphicsObject *PickObject ( int x, int y )
{
   int i;

   for(i = nextSlot - 1; i >= 0; i--)
   {
      if(displayList[i].x1 <= x && displayList[i].x2 >= x &&
         displayList[i].y1 <=  y && displayList[i].y2 >= y )
      {
         return  ( &displayList[i] );
      }
   }
   return NULL;
}

//-----------------------------------------------------------------------------
//FUNCTION:	MoveObject()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    void MoveObject(int x, int y)
//          
//RETURNS:      void
//DEPENDENCIES: displayList[]
//PRECONDITION: none 
//NOTES:        Move the object by erasing it in its current position,
//              updating the position information, and then drawing
//              the figure in its new position.        
//-----------------------------------------------------------------------------
void MoveObject(GraphicsObject *object, int dx, int dy )
{
   (*(object->func))(XtWindow(canvas), xorGC,
                 object->x1,
                 object->y1,
                 object->x2,
                 object->y2 );
   object->x1 += dx;
   object->y1 += dy;
   object->x2 += dx;
   object->y2 += dy;
  
   (*(object->func))(XtWindow ( canvas ), object->gc,
                 object->x1,
                 object->y1,
                 object->x2,
                 object->y2 );
}

//-----------------------------------------------------------------------------
//FUNCTIONS:	AddObject()
//              AddTextObject()
//              AddFreeLineObject()
//DATE:		1994
//REVISION:	May 6, 1997 - Chris Torrens & Cam Mitchner
//               - added FreeLine object list and Text Object list.
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    void AddObject(GraphicsObject *object)
//              void AddTextObject(TextObject *object)
//              void AddFreeLineObject(FreeLineObject *object)
//          
//RETURNS:      void
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        Check to make sure there is room in the object list, and add
//              the object if there is room.        
//-----------------------------------------------------------------------------
void AddObject(GraphicsObject *object)
{
   XGCValues values;

   /* Check for space. */
   if(nextSlot >= MAXOBJECTS)
   {
      fprintf(stderr, "Warning: Display List is full\n" );
      return;
   }

   /* Copy the information into the next available slot in the display list. */
   displayList[nextSlot].x1         = object->x1;
   displayList[nextSlot].y1         = object->y1;
   displayList[nextSlot].x2         = object->x2;
   displayList[nextSlot].y2         = object->y2;
   displayList[nextSlot].figureType = object->figureType;
   displayList[nextSlot].foreground = object->foreground;

   /*
    * Get a GC for this color. Using XtGetGC() ensures that
    * only as many GC's will be created as there are colors.
    * Objects with the same color will share a GC.
    */
   values.foreground = object->foreground;
   values.background = background;

   displayList[nextSlot].gc = 
           XtGetGC(canvas, GCBackground | GCForeground, &values);
   displayList[nextSlot].func = TypeToFunction(object->figureType);
   
  /*
   * For all figures expect a line, permantly correct the bounding
   * box to allowing easy picking. For a line, the coordinates
   * cannot be changed without changing the appearance of the figure.
   */

   if(object->figureType != LINE)
      FixDataOrdering(&displayList[nextSlot].x1,
                      &displayList[nextSlot].y1,
                      &displayList[nextSlot].x2,
                      &displayList[nextSlot].y2);

   /* Increment the next position index.*/
   nextSlot++;
}

void AddTextObject(TextObject *object)
{
   displayTextList[numTextObjects].x1 = object->x1;
   displayTextList[numTextObjects].y1 = object->y1;
   displayTextList[numTextObjects].str_size = object->str_size;
   strcpy(displayTextList[numTextObjects].str_buf, object->str_buf);
   displayTextList[numTextObjects].foreground = object->foreground;
   displayTextList[numTextObjects].gc = object->gc;

   numTextObjects++;
}

void AddFreeLineObject(FreeLineObject *object)
{
   displayFreeLineList[numFreeLines].num_points = object->num_points;
   displayFreeLineList[numFreeLines].points = object->points;
   displayFreeLineList[numFreeLines].foreground = object->foreground;
   displayFreeLineList[numFreeLines].gc = object->gc;

   numFreeLines++;
}

//-----------------------------------------------------------------------------
//FUNCTIONS:	StoreObject()
//              StoreTextObject()
//              StoreFreeLineObject()
//DATE:		1994
//REVISION:	May 6, 1997 - Chris Torrens & Cam Mitchner
//               - added FreeLine object list and Text Object list.
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    void StoreObject(Position startX, Position startY,
//                               Position lastX,  Position lastY,
//                               int drawingType)
//              void StoreTextObject(Position startX, Position startY,
//                                   int length, char *text, Pixel pixel,
//                                   GC gc)
//              void StoreFreeLineObject(int length, XPoint *pointList,
//                                       Pixel pixel, GC gc)
//          
//RETURNS:      void
//DEPENDENCIES: none
//PRECONDITION: none 
//NOTES:        Check to make sure there is room in the object list, and add
//              the object if there is room.        
//-----------------------------------------------------------------------------
void StoreObject(Position startX, Position startY, 
                 Position lastX,  Position lastY, int drawingType)
{
   GraphicsObject object;

   /* Don't store zero-sized figures. */
   if(startX == lastX && startY == lastY)
      return;

   /*
   ** Save the current values of all global variables 
   ** in a GraphicsObject structure.
   **/
   object.x1         = startX;
   object.y1         = startY;
   object.x2         = lastX;
   object.y2         = lastY;
   object.figureType = drawingType;
   object.foreground = currentForeground;

   /* Add the object to the display list. */ 
   AddObject(&object);
}

void StoreTextObject(Position x, Position y, int length, char *text, Pixel pixel, GC gc)
{
   TextObject object;

   object.x1 = x;
   object.y1 = y;
   object.str_size = length;
   strcpy(object.str_buf, text);
   object.foreground = pixel;
   object.gc = gc;

   AddTextObject(&object);  
}

void StoreFreeLineObject(int length, XPoint *pointList, Pixel pixel, GC gc)
{
    FreeLineObject object;

    object.num_points = length;
    object.points = pointList;
    object.foreground  = pixel;
    object.gc = gc;

    AddFreeLineObject(&object);
}
//----------------------------------------------------------------------------------
//FUNCTION:	StartRubberBand
//DATE:		May 6, 97
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Douglas Young
//PROGRAMMER:	
//INTERFACE:    static void StartRubberBand(Widget w,
//                                          XtPointer clientData,
//                                          XEvent *event,
//                                          Boolean *flag);
//	        - w: 
//              - clientData:
//              - *event:
//              - *flag:
//RETURNS:	void
//DEPENDENCIES: 
//PRECONDITION: 
//NOTES:
//----------------------------------------------------------------------------------

static void StartRubberBand(Widget    w,
                            XtPointer clientData,
                            XEvent   *event,
                            Boolean  *flag )
{

   /* Ignore any events other than a button press for mouse button one. */
   if(event->xbutton.button != Button1)
      return;

   if(currentFunction)
   {
      /*
      ** If there is a current drawing function, call it to
      ** start positioning the new figure.
      */
      XGCValues values;

      /* Get a GC for the current colors. */
      values.foreground = currentForeground;
      values.background = background;

      currentGC = XtGetGC(w,
                          GCBackground | GCForeground,
                          &values );

      /* Store the starting point and draw the initial figure. */
      if(currentFigureType == SOMETEXT)
      {
         static Widget textDialog = NULL;
         char message[80];
         char text[80];

         xPos = event->xbutton.x;
         yPos = event->xbutton.y;

         sprintf(message, "Inserting text at (%04d, %04d)\n", xPos, yPos);

         textDialog = CreateTextDialog((Widget)w, "textDialog", message,
                                       TextOkCallback, TextCancelCallback);
         XtVaSetValues(textDialog,
                       XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
                       NULL);

         XtManageChild(textDialog);
         XtAddCallback(textDialog, XmNokCallback,
                       positionTextCallback, NULL);
      }

      if(currentFigureType == FREEHAND)
      {
         displayPointList[0].x =(short) event->xbutton.x;
         displayPointList[0].y =(short) event->xbutton.y;
         numPoints = 1;
      }

      lastX = startX = event->xbutton.x;
      lastY = startY = event->xbutton.y;

      if(currentFigureType != FREEHAND)
      {
         (*(currentFunction))(XtWindow(w), xorGC,
                    startX, startY, 
                    lastX, lastY );
      }
    }
    else if(!currentFunction && event->xbutton.button == Button1)
    {
       /*
        * If there is no current function, treat Button1
        * as a pointer and find the graphics object the mouse
        * cursor is pointing to.
        */

       lastX = startX = event->xbutton.x;
       lastY = startY = event->xbutton.y;

       currentObject = PickObject(event->xbutton.x,
                                  event->xbutton.y );
       if(currentObject)
       {
          /*
           * If an object was found, any mouse motion will move
           * the object. Get a new graphics context whose colors
           * match the colors of the current object.
           */

           XGCValues values;
           
           XtReleaseGC ( w, xorGC );
           values.foreground = currentObject->foreground ^ 
                               background;
           values.function   = GXxor;

            /*
             * Change the color of the rubber band gc 
             */
            
            xorGC =  XtGetGC ( w,  GCForeground | GCFunction, 
                                   &values );
        }
    }
}

//-----------------------------------------------------------------------------
//FUNCTION:	positionTextCallback()
//DATE:		1994
//REVISION:	
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    void positionTextCallback(Widget w, XtPointer clientData,
//                                        XtPointer callData)
//          
//RETURNS:      void
//DEPENDENCIES: displayList[], SendText()
//PRECONDITION: none 
//NOTES:        Executes when the user has finished typing in text in the
//              Text dialog box.  The text is placed at the specified location        
//-----------------------------------------------------------------------------
void positionTextCallback(Widget w, XtPointer clientData, XtPointer callData)
{
   char *message = (char *)clientData;
  
   XDrawString(XtDisplay(canvas), XtWindow(canvas), currentGC, xPos, yPos,
             textObject, strlen(textObject));
   
   displayTextList[numTextObjects].x1 = xPos;
   displayTextList[numTextObjects].y1 = yPos;
   strcpy(displayTextList[numTextObjects].str_buf, textObject);
   displayTextList[numTextObjects].str_size = strlen(textObject);
   displayTextList[numTextObjects].foreground = currentForeground;
   displayTextList[numTextObjects].gc = currentGC;   

   SendText(&(displayTextList[numTextObjects]));

   numTextObjects++;
}

//----------------------------------------------------------------------------------
//FUNCTION:	TrackRubberBand
//DATE:		May 6, 97
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    static void StartRubberBand(Widget w,
//                                          XtPointer clientData,
//                                          XEvent *event,
//                                          Boolean *flag);
//	        - w: 
//              - clientData:
//              - *event:
//              - *flag:
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:        Tracks the mouse position as button1 is held down.
//----------------------------------------------------------------------------------
static void TrackRubberBand ( Widget    w, 
                              XtPointer clientData,
                              XEvent   *event,
                              Boolean  *flag )
{ 
   /*
    * If there is a current drawing function, handle the 
    * rubberbanding of the new figure. Otherwise, if there
    * is a currently selected object, move it, tracking the
    * cursor position.
    */

    if ( currentFunction && 
        ( event->xmotion.state & Button1Mask ) )
    {
       
      if(currentFigureType == FREEHAND)
      { 
	if(numPoints < MAXPOINTS)           
        {
	  displayPointList[numPoints].x =(short) event->xmotion.x;
	  displayPointList[numPoints].y = (short)event->xmotion.y;
	  numPoints++;
	  XDrawLines(display, XtWindow (w), currentGC,displayPointList,
		      numPoints,CoordModeOrigin);
	}
      }
      
      else
      {

	/* Erase the previous figure. */
        ( * ( currentFunction ) ) ( XtWindow ( w ), xorGC,
                                    startX, startY, 
                                    lastX, lastY );
      }

      /* Update the last point. */
      lastX  =  event->xmotion.x;
      lastY  =  event->xmotion.y;
        
 
      /* Draw the figure in the new position. */
      if(currentFigureType == FREEHAND)
      {
	  ( * ( currentFunction ) ) ( XtWindow ( w ), currentGC,
				      startX, startY, 
				      lastX, lastY );
      }
      else
      {

	  ( * ( currentFunction ) ) ( XtWindow ( w ), xorGC,
				      startX, startY, 
				      lastX, lastY );
      }   
      
    }
    else if  ( currentObject    && 
               !currentFunction && 
               event->xmotion.state & Button1Mask )
    {
        MoveObject ( currentObject, 
                     event->xmotion.x - lastX, 
                     event->xmotion.y - lastY );
        
        lastX  =  event->xmotion.x;
        lastY  =  event->xmotion.y;
    }
}

//----------------------------------------------------------------------------------
//FUNCTION:	EndRubberBand
//DATE:		May 6, 97
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    static void EndRubberBand(Widget w,
//                                          XtPointer clientData,
//                                          XEvent *event,
//                                          Boolean *flag);
//RETURNS:	void
//DEPENDENCIES: displayFreeLineList[], PackAndSend(), StoreObject() 
//PRECONDITION: none
//NOTES:        Executes when the user has released button one of the mouse
//              to complete a drawing operation.  This finalizes the operation
//              begun with StartRubberBand().  If there is a current drawing
//              function, clear the last XOR image and draw the final figure.
//----------------------------------------------------------------------------------                                
static void EndRubberBand ( Widget    w,
                            XtPointer clientData,
                            XEvent   *event,
                            Boolean   *flag )
{

    XPoint *tempPointList,*index;
    int i;

    if ( currentFunction && 
         event->xbutton.button == Button1 )
    {
        
        tempPointList = (XPoint*)malloc((sizeof(XPoint)*numPoints));
	index = tempPointList;
        if(currentFigureType == FREEHAND)
	{
      
	  for(i=0;i<numPoints;i++)
	  { 
	    (index +i)->x = displayPointList[i].x;
	    (index +i)->y = displayPointList[i].y;
	  }	  
	  
	  XDrawLines(display, XtWindow (w), currentGC, tempPointList,
		      numPoints,CoordModeOrigin);
	  numFreeLines++;
          displayFreeLineList[numFreeLines-1].num_points = numPoints;
	  displayFreeLineList[numFreeLines-1].points = tempPointList;
	  displayFreeLineList[numFreeLines-1].foreground  = currentForeground;
	  displayFreeLineList[numFreeLines-1].gc = currentGC;

	  SendLine(tempPointList,numPoints);
	  numPoints = 0;
	  tempPointList = NULL;
	  index = NULL;
	}

	else
	{
	  /*
	   * Erase the XOR image. 
	   */
        
	  ( * ( currentFunction ) ) ( XtWindow ( w ), xorGC,
				      startX, startY, 
				      lastX,  lastY );
        
	  /*
	   * Draw the figure using the normal GC. 
	   */
        
	  ( * ( currentFunction ) ) ( XtWindow ( w ), currentGC,
				      startX, startY, 
				      event->xbutton.x, 
				      event->xbutton.y );
        
	  /*
	   * Update the data, and store the object in 
	   * the display list.
	   */
	  
	  lastX = event->xbutton.x;
	  lastY = event->xbutton.y;
        
	  PackandSend(currentFigureType,startX,startY,lastX,lastY);
	  StoreObject(startX, startY, lastX, lastY, currentFigureType);
        
	  currentObject = PickObject ( event->xbutton.x,
				       event->xbutton.y );
	}
    }

    else if  ( currentObject    && 
               !currentFunction && 
               event->xbutton.button == Button1 )
    {

       /*
        * If the operation was a drag, move the figure
        * to its final position.
        */

        MoveObject ( currentObject, 
                     event->xbutton.x - lastX,  
                     event->xbutton.y - lastY );

       /*
        * Force a complete redraw.
        */
        
        XClearArea ( display, XtWindow ( w ), 0, 0, 0, 0, TRUE );
    }
}
                                                
//----------------------------------------------------------------------------------
//FUNCTION:	Pack()
//              PackLine()
//              PackText()
//DATE:		May 6, 97
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    static char *Pack(GraphicsObject *object)
//              static char *PackLine(FreeLineObject *line)              
//              static char *PackText(TextObject *textObject)
//
//RETURNS:	pointer to char of the converted object to a string.
//DEPENDENCIES: displayFreeLineList[], PackAndSend(), StoreObject() 
//PRECONDITION: none
//NOTES:        These functions take care of 'packing' an object
//              into its essential properties for storage.        
//----------------------------------------------------------------------------------                                
static char *Pack (GraphicsObject *object)
{

    /* Convert an object to a string. */

    static char buf[1000];
    XColor      color;

    color.pixel = object->foreground;
    XQueryColor ( display, colormap, &color );
    
    sprintf ( buf, "%d %d %d %d %d %d %d %d",
              object->x1, object->y1, 
              object->x2, object->y2,
              object->figureType,
              color.red, color.green, color.blue );
    
    return ( buf );
}

static char *PackLine ( FreeLineObject *line)
{
   static char buf[(MAXPOINTS*4)];
   char smallbuf[12];
   int i;

   buf[0] = '\0';

   for(i = 0; i < line->num_points; i++)
   {
     sprintf(smallbuf, "%d %d\n", (line->points+i)->x, (line->points+i)->y);
     strcat(buf, smallbuf);
   }

   return buf;
}

static char *PackText(TextObject *textObject)
{
   static char buf[100] = {'\0'};
   XColor      color;

   color.pixel = textObject->foreground;
   XQueryColor(display, colormap, &color);

   sprintf(buf, "%d %d %d %s %hd %hd %hd",
                textObject->x1,
                textObject->y1,
                textObject->str_size,
                textObject->str_buf,
                color.red,
                color.green,
                color.blue);

   return buf;
}

//----------------------------------------------------------------------------------
//FUNCTION:	Unpack()
//              UnpackLine()
//              UnpackText()
//DATE:		May 6, 97
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Douglas Young
//PROGRAMMER:	Douglas Young
//INTERFACE:    void Unpack(char *str, GraphicsObject *gobj)
//              static void UnpackText(const char *buf)              
//              static void UnpackLine(FILE * fp, char buf[80])
//
//RETURNS:	void
//DEPENDENCIES: displayFreeLineList[], PackAndSend(), StoreObject() 
//PRECONDITION: none
//NOTES:        These functions take care of un-doing what Pack did to the object
//              lists.  Each object is unpacked into its proper list with its
//              properties restored.        
//----------------------------------------------------------------------------------
void Unpack(char *str, GraphicsObject *gobj)
{

   /*
    * Convert a string to an object
    */

    XColor                color;

    sscanf ( str, "%d %d %d %d %d %hd %hd %hd", 
             &gobj->x1, &gobj->y1, 
             &gobj->x2, &gobj->y2, 
             &gobj->figureType, 
             &(color.red),  &(color.green), &(color.blue) );

   /*
    * Get the pixel that corresponds to the given color.
    */

    XAllocColor ( display, colormap, &color );
    gobj->foreground = color.pixel;

}

static void UnpackText(const char *buf)
{
    XColor color;
    XGCValues values;

    sscanf(buf, "%hd %hd %hd %ld %ld %ld %80c", 
                      &color.red, &color.green, &color.blue, 
                      &(displayTextList[numTextObjects].x1),
                      &(displayTextList[numTextObjects].y1),
                      &(displayTextList[numTextObjects].str_size),
                      &(displayTextList[numTextObjects].str_buf));

    XAllocColor(display, colormap, &color);
    displayTextList[numTextObjects].foreground = color.pixel;
    values.foreground = displayTextList[numTextObjects].foreground;
    values.background = background;
    
    displayTextList[numTextObjects].gc = XtGetGC ( canvas, 
                                                   GCBackground | GCForeground,
                                                   &values );   
    numTextObjects++;
}

static void UnpackLine(FILE * fp, char buf[80])
{

    XColor color;
    XGCValues values;
    int i;
    
    sscanf(buf,"%hd %hd %hd %d",&color.red,&color.green, &color.blue,
	   &(displayFreeLineList[numFreeLines].num_points));
 
    displayFreeLineList[numFreeLines].points = (XPoint*)malloc(sizeof(XPoint)*
		      (displayFreeLineList[numFreeLines].num_points));
  
    for(i=0;i<displayFreeLineList[numFreeLines].num_points;i++)
    {
      fgets(buf,80,fp);
      sscanf(buf,"%hd %hd",&((displayFreeLineList[numFreeLines].points+i)->x),
	     &((displayFreeLineList[numFreeLines].points+i)->y));
    }

	
    XAllocColor(display,colormap,&color);
    displayFreeLineList[numFreeLines].foreground = color.pixel;
     
    values.foreground = displayFreeLineList[numFreeLines].foreground;
    values.background = background;
    
    displayFreeLineList[numFreeLines].gc = XtGetGC ( canvas, 
                                         GCBackground | GCForeground,
                                         &values ); 
    numFreeLines++;
}

//----------------------------------------------------------------------------------
//FUNCTION:	SaveData()
//DATE:		May 6, 1997
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Chris Torrens & Cam Mitchner
//PROGRAMMER:	Chris Torrens & Cam Mitchner
//INTERFACE:    void SaveData()
//RETURNS:	void
//DEPENDENCIES: SaveDataCallback, CancelDialogCallback 
//PRECONDITION: none
//NOTES:        Creates a file selection dialog box in which the user may select
//              a file that contains object data.  Calls SaveDataCallback if 'OK'
//              is selected.
//----------------------------------------------------------------------------------
void SaveData()
{
    FILE  *fp;
    static Widget dialog = NULL;
        
   /*
    * Create the dialog if it doesn't already exist. Install
    * callbacks for OK and Cancel actions.
    */

    if ( !dialog )
    {
         dialog = XmCreateFileSelectionDialog ( canvas, "saveDialog",
                                                NULL, 0 );

         XtAddCallback ( dialog, XmNokCallback,
                         SaveDataCallback,  ( XtPointer ) NULL );
         XtAddCallback ( dialog, XmNcancelCallback,
                         CancelDialogCallback,  ( XtPointer ) NULL );
     }
        
     XtManageChild ( dialog );

 }
            
//----------------------------------------------------------------------------------
//FUNCTION:	SaveDataCallback()
//DATE:		May 6, 1997
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Chris Torrens & Cam Mitchner
//PROGRAMMER:	Chris Torrens & Cam Mitchner
//INTERFACE:    void SaveDataCallback()
//RETURNS:	void
//DEPENDENCIES: GraphicsObject, FreeLineObject, TextObject, Pack, PackText,
//              PackFreeLine.
//PRECONDITION: none
//NOTES:        Executes when the user chooses OK after selecting a file.
//              Objects are 'packed' and then stored in an ASCII file using the
//              specified 'pack' function for each object.  Each object type is
//              separated by headings: FREELINE_SECTION TEXT_SECTION
//----------------------------------------------------------------------------------
void SaveDataCallback ( Widget    w,
                        XtPointer clientData,
                        XtPointer callData )
{
   FILE    *fp;
   char    *fileName;
   XmString xmstr;
   int      i;
   XColor   color;

   /* Remove the dialog from the screen. */
   XtUnmanageChild(w);
    
   /* Retrieve the currently selected file. */
   XtVaGetValues (w, XmNdirSpec, &xmstr, NULL);

   /* Make sure a file was selected. */
   if(!xmstr)
      return;

   /* Retrieve the name of the file as ASCII. */
   XmStringGetLtoR(xmstr, XmFONTLIST_DEFAULT_TAG, &fileName);

   /*Try to open the file for writing. */
   if((fp = fopen(fileName, "w" )) == NULL)
      return;

   /* Loop though the display list, writing each object to file */
   for(i = 0; i < nextSlot; i++)
   {
      GraphicsObject *object = &( displayList[i] );
      fprintf(fp, "%s\n", Pack(object));
   }
    
   fprintf(fp, "FREELINE_SECTION\n");

   for(i = 0; i < numFreeLines; i++)
   {
      FreeLineObject *line = &(displayFreeLineList[i]);
      color.pixel = line->foreground;
      XQueryColor(display, colormap, &color);

      /* Store the RGB colors, number of points, and the line object. */	
      fprintf(fp, "%hd %hd %hd %hd\n%s", color.red, color.green, color.blue,
                                          line->num_points, PackLine(line));
   }

   fprintf(fp, "TEXT_SECTION\n");
                
   for(i = 0; i < numTextObjects; i++ )
   {
      TextObject *text = &(displayTextList[i]);
      color.pixel = text->foreground;
      XQueryColor( display, colormap, &color );
 
      /* Store the beginning (x,y) coordinates and the string size */ 
      fprintf(fp, "%hd %hd %hd %hd %hd %hd %s\n",
                color.red, color.green, color.blue,
                text->x1, text->y1, text->str_size, text->str_buf);
    }
    fclose(fp);
}
                                
//----------------------------------------------------------------------------------
//FUNCTION:	LoadData()
//DATE:		May 6, 1997
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Chris Torrens & Cam Mitchner
//PROGRAMMER:	Chris Torrens & Cam Mitchner
//INTERFACE:    void LoadData()
//RETURNS:	void
//DEPENDENCIES: LoadDataCallback, CancelDialogCallback 
//PRECONDITION: none
//NOTES:        Create a file selection dialog box in which the user may select
//              a file that contains object data.  Calls LoadDataCallback if 'OK'
//              is selected.
//----------------------------------------------------------------------------------
void LoadData()
{
   static Widget dialog = NULL;
    
   /* Create a file selection dialog if it doesn't already exist */
   if (!dialog)
   {
      dialog = XmCreateFileSelectionDialog(canvas, 
                                           "loadDialog",
                                           NULL, 0);
      XtAddCallback(dialog, XmNokCallback, LoadDataCallback, (XtPointer)NULL);
      XtAddCallback(dialog, XmNcancelCallback, CancelDialogCallback, (XtPointer)NULL);
   }
    
   /* Display the dialog */
   XtManageChild ( dialog );
}
            
//----------------------------------------------------------------------------------
//FUNCTION:	LoadDataCallback()
//DATE:		May 6, 1997
//REVISION:	Chris Torrens and Cam Mitchner
//                - add text and freehand functionality
//DESIGNER:	Chris Torrens & Cam Mitchner
//PROGRAMMER:	Chris Torrens & Cam Mitchner
//INTERFACE:    void LoadDataCallback(Widget w, XtPointer clientData,
//                                              XtPointer callData)
//RETURNS:	void
//DEPENDENCIES: GraphicsObject, FreeLineObject, TextObject, Pack, PackText,
//              PackFreeLine.
//PRECONDITION: none
//NOTES:        Executes when the user chooses OK after selecting a file.
//              Objects are 'Unpacked' from an ASCII file and then stored in its
//              specified object list.  Each object type is
//              separated by headings: FREELINE_SECTION TEXT_SECTION
//----------------------------------------------------------------------------------
void LoadDataCallback(Widget w, XtPointer clientData, XtPointer callData)
{
   /*
    * This function is called if a user selects a file to load.
    * Extract the selected file name and read the data.
    */

   FILE          *fp;
   char          *fileName;
   XmString       xmstr;
   GraphicsObject object;
   char           buf[80];

   /* Remove the dialog from the screen. */
   XtUnmanageChild(w);
    
   /* Retrieve the selected file */
   XtVaGetValues(w, XmNdirSpec, &xmstr, NULL);

   /* Confirm that a file was selected */
   if(!xmstr)
      return;

   /* Retrieve an ASCII string from the compound string. */
   XmStringGetLtoR ( xmstr, XmFONTLIST_DEFAULT_TAG, &fileName );

   if((fp = fopen(fileName, "r" )) == NULL)
      return;
    
   ClearWindow(canvas);
   nextSlot = 0;

   /* Read each object into the displayList using Unpack */
   if((fgets(buf, 80, fp)) != NULL)
   {
      while(strncmp(buf, "FREELINE_SECTION", 16))
      {
         Unpack(buf, &object);
         AddObject ( &object );
         if((fgets(buf,80,fp))==NULL)
         { 
            fclose(fp);
            return;
	}
      }
    }

    if((fgets(buf,80,fp))!=NULL)
    {
      while (strncmp(buf, "TEXT_SECTION",12))
      {
	UnpackLine(fp,buf);

	if((fgets(buf,80,fp))==NULL)
	{
	  fclose(fp);
	  return;
	}
      }     
    }

    while((fgets(buf, 80, fp)) != NULL)
       UnpackText(buf);
    
   /*
    * Force a redraw.
    */
    XClearArea ( display, XtWindow ( canvas ), 0, 0, 0, 0, TRUE );
    
    fclose ( fp );
}

void CancelDialogCallback ( Widget    w, 
                            XtPointer clientData,
                            XtPointer callData )
{
    XtUnmanageChild ( w );
}

/* Accessor functions for static global variables.  by Dr. Kloooge */
short getStartX(void) { return startX; }
short getStartY(void) { return startY; }
short getLastX(void)  { return lastX;  }
short getLastY(void)  { return lastY;  }

void setStartX(short x) { startX = x; }
void setStartY(short y) { startY = y; }
void setLastX(short x)  { lastX = x;  }
void setLastY(short y)  { lastY = y;  }

void setText(char *text) { strcpy(textObject, text); }

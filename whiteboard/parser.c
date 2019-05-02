//-----------------------------------------------------------------------------
//FILE:	       parser.c
//DATE:	       May 6, 97
//REVISION:     
//DESIGNER:    Chris Torrens and Cam Mitchner
//PROGRAMMER:  Chris Torrens and Cam Mitchner
//NOTES:       contains functions for sending graphic objects to the network
//             layer and decoding messages from remote hosts arriving via the
//             network layer.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <Xm/Xm.h>
#include "baerg.h"
#include "parser.h"
#include "graphics.h"
#include "socket.h"

extern FreeLineObject displayFreeLineList[MAXLINES];
  
//-----------------------------------------------------------------------------
//FUNCTION:	commandParser
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	void commandParser(Display *display, Widget mycanvas,
//                GC mycurrentGC, char data[], int len, char type)
//              -display: pointer to the Display we are drawing to
//              -mycanvas: window to draw to
//              -mycurrentGC: current Graphics Context to use for drawing
//              -data[]: data part of Common Whiteboard Transfer Protocol packet
//                received
//              -len: length (16-bit words) of data packet (Max 128)
//              -type: CWTP protocol operation 
//RETURNS:	none
//DEPENDENCIES: calls get16Bit function and XWindow drawing functions
//PRECONDITION: len, type and data[] must be valid CWTP protocol values
//              no other functions can be accessing graphic object lists or
//              startX,startY,lastX,lastY while this function is executing
//NOTES:        decodes drawing operation type. decodes data portion of 
//              message. Calls drawing function using decoded data. Stores
//              drawn object in global object list.
//-----------------------------------------------------------------------------
void commandParser(Display *display, Widget mycanvas, char data[], int len, char type, XColor color, int drawingType)
{
  unsigned short  x1, y1, x2, y2, length, width, height;
  char text[80];
  int counter = 0,number = 0, i;
  XPoint *tempPointList, *index;
  GC mycurrentGC;
  XGCValues values;
  Colormap mycolormap;

  length = width = height = x1 = x2 = y1 = y2 = 0;

  //decode the drawing operation
  XtVaGetValues(mycanvas, XmNcolormap, &mycolormap, NULL);
  XAllocColor (display, mycolormap,&color); 
  values.foreground = color.pixel;
  values.background = background;

  mycurrentGC = XtGetGC(mycanvas, GCBackground | GCForeground, &values);

  switch(type)
  {
    case B_OP_POINTS:   //draw freehand
      number = len/4; //16bits per coord, 2 coords per point (x and y)

      tempPointList = (XPoint*)malloc((sizeof(XPoint)*number));
      index = tempPointList;

      //decode 8-bit chars into 16-bit shorts and store in temporary XPoint list
      for (i=0;i<number;i++)
      {
	(index + i)->x = ntohs(*((unsigned short*)(data+counter)));
	counter+=2;
	(index + i)->y = ntohs(*((unsigned short*)(data+counter)));
        counter+=2;
      }
   
      //draw freehand lines to screen
      XDrawLines(display, XtWindow(mycanvas), mycurrentGC, tempPointList,
		 number, CoordModeOrigin);

      /* Add new free line object to the freeline object list */ 
      StoreFreeLineObject(number, tempPointList, color.pixel, mycurrentGC);
     
      tempPointList = NULL;
      index = NULL;
      break;

    case B_OP_LINE:  //draw line
      //decode 8-bit chars into 16-bit values x1,y1,x2,y2
      x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      x2 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y2 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      //draw line and store line in graphic object array
      XDrawLine(display,XtWindow(mycanvas),mycurrentGC,x1,y1,x2,y2);
      StoreObject(x1, y1, x2, y2, drawingType);
      break;

    case B_OP_RECT:
      x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      width = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      height = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      XDrawRectangle(display,XtWindow(mycanvas),mycurrentGC,x1,y1,width,height);
      StoreObject(x1, y1, x2 + width, y2 + height, drawingType);
      break;

    case B_OP_ELLIPSE:
      x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      width = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      height = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
 
      XDrawArc(display,XtWindow(mycanvas),mycurrentGC,x1,y1,width,height,0,64*360);
      StoreObject(x1, y1, x2 + width, y2 + height, drawingType);
      break;

    case B_OP_TEXT:
      x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      length = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
   
      XDrawString(display, XtWindow(mycanvas), mycurrentGC, 
                  x1, y1, data + counter, length);

      StoreTextObject(x1, y1, length, data + counter, color.pixel, mycurrentGC); 
      break;

    case B_OP_FILL_RECT:
      x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      width = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      height = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      XFillRectangle(display,XtWindow(mycanvas),mycurrentGC,x1,y1,width,height);
      StoreObject(x1, y1, x2 + width, y2 + height, drawingType);
      break;

    case B_OP_FILL_ELLIPSE:
      x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      width = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      height = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      XFillArc(display, XtWindow(mycanvas), mycurrentGC,x1,y1, width, height,0,64*360);
      StoreObject(x1, y1, x2 + width, y2 + height, drawingType);
      break;

    case B_OP_ERASE: //erase -- use filled rectangle in background color
/*    x1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      y1 = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      width = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      height = ntohs(*((unsigned short*)(data+counter)));
      counter+=2;
      XFillRectangle(display,XtWindow(mycanvas),xorGC, x1, y1, width, height);
      StoreObject(x1, y1, x2 + width, y2 + height, drawingType);
*/    break;

    default:
      printf("commandParser: Illegal operation code from remote host!\n");
      break;
  }
 
}

//-----------------------------------------------------------------------------
//FUNCTION:	PackandSend
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	void PackandSend(int type, int x1, int y1, int x2, int y2)
//	        - type: CWTP protocol operation to be sent
//              - x1,y1: starting coordinates of object to send
//              - x2,y2: ending coordinates of object
//RETURNS:	none
//DEPENDENCIES: SockManager::Instance and SockManager::broadcast_operation
//PRECONDITION: 
//NOTES:        parses all CWTP drawing messages except for freehand lines and
//              text. Creates data for CWTP packet and sends to network layer
//              along.
//              with operation type and length of data for operation.
//-----------------------------------------------------------------------------
void PackandSend(int type, unsigned short x1, unsigned short y1,
                 unsigned short x2, unsigned short y2)
{
    char data[512];
    int  len = 0;
    unsigned short temp = 0;
    unsigned short x_start = x1;
    unsigned short y_start = y1;
    unsigned short x_end   = x2;
    unsigned short y_end   = y2;
    unsigned short width = 0;
    unsigned short height = 0;
    unsigned char operation = 0;
    SockManager *smanager = SockManager::Instance();

    if(y_end < y_start && type != LINE)
    {
       temp = y_end;
       y_end = y_start;
       y_start = temp;
    }
    if(x_end < x_start && type != LINE)
    {
       temp = x_end;
       x_end = x_start;
       x_start = temp;
    }

    x_start = htons(x_start);
    y_start = htons(y_start);
    x_end = htons(x_end);
    y_end = htons(y_end);
    width = htons((unsigned short)abs(x2-x1));
    height = htons((unsigned short)abs(y2-y1));

    memset(data, 0, 512);

    switch(type)
    {
    case LINE:
        operation = B_OP_LINE;
	memcpy(data, (char*)&x_start, 2);
	memcpy(data+2, (char*)&y_start, 2);
	memcpy(data+4, (char*)&x_end, 2);
	memcpy(data+6, (char*)&y_end, 2);
	len = 8;
        break;

    case RECTANGLE:
        operation = B_OP_RECT;
	memcpy(data, (char*)&x_start, 2);
	memcpy(data+2, (char*)&y_start, 2);
	memcpy(data+4, (char*)&width, 2);
	memcpy(data+6, (char*)&height, 2);
	len = 8;
        break;

    case CIRCLE:
        operation = B_OP_ELLIPSE;
	memcpy(data, (char*)&x_start, 2);
	memcpy(data+2, (char*)&y_start, 2);
	memcpy(data+4, (char*)&width, 2);
	memcpy(data+6, (char*)&height, 2);
	len = 8;
        break;

    case FILLEDRECTANGLE:
        operation = B_OP_FILL_RECT;
	memcpy(data, (char*)&x_start, 2);
	memcpy(data+2, (char*)&y_start, 2);
	memcpy(data+4, (char*)&width, 2);
	memcpy(data+6, (char*)&height, 2);
	len = 8;
        break;

    case FILLEDCIRCLE:
        operation = B_OP_FILL_ELLIPSE;
	memcpy(data, (char*)&x_start, 2);
	memcpy(data+2, (char*)&y_start, 2);
	memcpy(data+4, (char*)&width, 2);
	memcpy(data+6, (char*)&height, 2);
	len = 8;
        break;

    case ERASE:
        operation = B_OP_ERASE;
	memcpy(data, (char*)&x_start, 2);
	memcpy(data+2, (char*)&y_start, 2);
	memcpy(data+4, (char*)&width, 2);
	memcpy(data+6, (char*)&height, 2);
	len = 8;
        break;

    case CLEARSCREEN:
        operation = B_OP_CLEARSCREEN;
	len = 0;
        break;

    default:
        printf("Unknown operation type %d\n", type);
        return;
    }

    smanager->broadcast_operation(operation,data,len);
}

//-----------------------------------------------------------------------------
//FUNCTION:	SendLine
//DATE:		May 6, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	void SendLine(XPoint *tempPointList, int numPoints)
//	        - tempPointList:pointer to list of XPoints making up the line to 
//                be sent
//              - numPoints: number of XPoints making up list
//RETURNS:	none
//DEPENDENCIES: SockManager::Instance and SockManager::broadcast_operation
//PRECONDITION: 
//NOTES:        sends lines out in 128-point segments
//-----------------------------------------------------------------------------
void SendLine(XPoint *tempPointList,int numPoints)
{
  char  data[512];
  char *ptr = data;
  SockManager *smanager = SockManager::Instance();
  int i = 0;
  int count = 0;
  int pointsSent = 0;
  int pointsLeft = numPoints;
  XPoint* currentPoint = tempPointList;
  unsigned short x_val = 0;
  unsigned short y_val = 0;

  while(pointsLeft > 0)
  {
    count = (pointsLeft > 127) ? 127 : pointsLeft;

    for(i = 0; i < count; i++)
    {
      x_val = htons(currentPoint->x);
      y_val = htons(currentPoint->y);
      memcpy(ptr    , (char *)&x_val, 2);
      memcpy(ptr + 2, (char *)&y_val, 2);
      ptr += 4;
      currentPoint++;
    }
  
    pointsSent += count;
    pointsLeft = numPoints - pointsSent;

    if (pointsLeft > 0)
    {
      currentPoint--;
      numPoints += 1;
      pointsLeft += 1;
    }

    smanager->broadcast_operation(B_OP_POINTS, data, ptr-data);


    for(i = 1; i < 10000000; i++)
       ;
   
    ptr = data;
  }
}

//-----------------------------------------------------------------------------
//FUNCTION:	SendText
//DATE:		May 7, 97
//REVISION:	
//DESIGNER:	Chris Torrens and Cam Mitchner
//PROGRAMMER:	Chris Torrens and Cam Mitchner
//INTERFACE:	void SendText(TextObject *textObject)
//	        - textObject: pointer to a TextObject 
//RETURNS:	none
//DEPENDENCIES: SockManager::Instance and SockManager::broadcast_operation
//PRECONDITION: 
//NOTES:        sends text out.  
//-----------------------------------------------------------------------------
void SendText(TextObject *textObject)
{
   char data[512];
   char str_buf[80];
   unsigned short x1, y1, str_size; 
 
   strcpy(str_buf, textObject->str_buf);
   x1 = textObject->x1;
   y1 = textObject->y1;
   str_size = textObject->str_size;

   SockManager *smanager = SockManager::Instance();

   memset(data, 0, 512);

   memcpy(data,   (char *)&x1, 2);
   memcpy(data+2, (char *)&y1, 2);
   memcpy(data+4, (char *)&str_size, 2);
   memcpy(data+6, (char *)&str_buf, strlen(str_buf));

   smanager->broadcast_operation(B_OP_TEXT, data, 6 + strlen(textObject->str_buf));
}

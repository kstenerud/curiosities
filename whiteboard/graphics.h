/* graphics.h: Header file for graphics.c */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <stdio.h>
#include "draw.h"
#include "baerg.h"
 
#define MAXOBJECTS 1000 /* The most objects the displayList can hold */
#define MAXTEXT 100 /*The most text objects the displayTestList can hold*/
#define MAXLINES 200 /*The most line objects to store in displayLineList*/
#define MAXPOINTS 2000 /*The most XPoints we can have in a Free Line*/

#define MAXTEXTBUFFERSIZE 80

void positionTextCallback(Widget w, XtPointer clientData, XtPointer callData);
void setText(char *textObject);
void SetCurrentColor(Pixel pixel);
short getStartX(void);
short getStartY(void);
short getLastX(void);
short getLastY(void);
void setStartX(short);
void setStartY(short);
void setLastX(short);
void setLastY(short);

/*
 * Define a standard for for all drawing function used in this module
 * to allow all operation to be invoked with the same API.
 */

typedef void ( *DrawingFunction ) ( Window, GC, int, int, int, int );

/*
 * Define a data structure that describes a single visible 
 * object in the display list.
 */

typedef struct{
    int             x1, y1, x2, y2;
    DrawingFunction func;
    int             figureType;
    Pixel           foreground;
    GC              gc;

} GraphicsObject;

/* Define a structure that describes a text box on the screen*/

typedef struct{
    int            x1, y1;
    int            str_size;
    char           str_buf[MAXTEXTBUFFERSIZE];
    Pixel          foreground;
    GC             gc;
} TextObject;

typedef struct
{
    int            num_points;
    XPoint*        points;
    Pixel          foreground;
    GC             gc;
} FreeLineObject;


/*
 * Various variables used within this module. All are global to
 * functions in this file, but static, so as to be hidden from
 * functions in other parts of the program.
 */
static int              numPoints       = 0;

static int              numFreeLines    = 0;

static int              numTextObjects  = 0;

static int              nextSlot        = 0;    /* Next free spot 
                                                   in displayList */
static GraphicsObject  *currentObject   = NULL;/* Pointer to currently
                                                  selected object */
static TextObject      *currentTextObject = NULL;

static FreeLineObject  *currentFreeObject = NULL;

static XPoint          *currentXPoint = NULL;

static DrawingFunction  currentFunction = NULL; /* Function that draws
                                                current figure type */
static int              currentFigureType  = 0; /* Current figure type
                                                   being drawn. */
static Display         *display           = NULL;
static Colormap         colormap;
static GC               currentGC         = NULL;
static GC               xorGC             = NULL;
static Pixel            currentForeground = 0;
static Pixel            background        = 5;
static Widget           canvas            = NULL;
static Position startX = 0, startY = 0, lastX = 0, lastY = 0;

/*
 * Functions that draw each figure type using the same arguments
 */

static void  DrawLine ( Window, GC, int, int, int, int );
static void  DrawCircle ( Window, GC, int, int, int, int );
static void  DrawRectangle ( Window, GC, int, int, int, int );
static void  DrawFilledCircle ( Window, GC, int, int, int, int );
static void  DrawFilledRectangle ( Window, GC, int, int, int, int );
static void  DrawFreeHand ( Window, GC, int, int, int, int);
static void  NewText ( Window, GC, int, int, int, int);
static void  Erase ( Window, GC, int, int, int, int);
void FixDataOrdering ( int *x, int *y, int *x2, int *y2 );

/*
 * Functions for "flattening" and "unflattening" an object
 */

void Unpack ( char *str, GraphicsObject *gobj );
static void           UnpackLine (FILE *,char[]);
static void           UnpackText(const char *);
static char           *Pack ( GraphicsObject *object );
static char           *Packline(FreeLineObject *line);

/*
 * Callback functions
 */

void draw_callback(unsigned char id, unsigned char operation, char*data,int len);

void error_callback(char* error, char* desc);

void ErrorOkCallback(Widget , XtPointer , XtPointer );
 
static void Redisplay ( Widget    w, 
                         XtPointer clientData, 
                         XtPointer callData );

static void StartRubberBand ( Widget     w, 
                               XtPointer  clientData, 
                               XEvent    *event, 
                               Boolean   *flag );
static void TrackRubberBand ( Widget     w, 
                               XtPointer  clientData, 
                               XEvent    *event, 
                               Boolean   *flag );
static void EndRubberBand ( Widget     w, 
                             XtPointer  clientData, 
                             XEvent    *event, 
                             Boolean   *flag );
static void LoadDataCallback ( Widget    w, 
                               XtPointer  clientData, 
                               XtPointer  callData );
static void CancelDialogCallback ( Widget     w, 
                                   XtPointer  clientData, 
                                   XtPointer  callData );
static void SaveDataCallback ( Widget     w, 
                               XtPointer  clientData, 
                               XtPointer  callData );
/*
static void GetTextCallback( Widget       w,
			     XtPointer    clientData,
			     XtPointer    callData);
*/

/*
 * Functions for storing, removing, moving and picking objects
 * in the display list.
 */

void StoreObject ( Position,Position,Position,Position, int );
void StoreTextObject( Position, Position, int, char *, Pixel, GC);
void StoreFreeLineObject( int, XPoint *, Pixel, GC);

void AddObject(GraphicsObject *);
void AddTextObject(TextObject *);
void AddFreeLineObject(FreeLineObject *);

static void MoveObject ( GraphicsObject *object, int x, int y );
static GraphicsObject *PickObject ( int x, int y );
void ClearWindow(Widget w);                                                             
#endif

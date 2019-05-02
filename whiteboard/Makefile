##
## Makefile for simple Client/Server Whiteboard project
##
##    Original code taken from:
##       The X Window System: Program and Applications with Xt
##                            2nd Edition
##          by: Douglas Young
##
##    Revisions:
##        April 1997 - included help.c and MenuSupport.c
##        May   1997 - included connectDialog.c addtextdialog.c
##                        mousetracker.c wprintf.c parser.c
##
   
CC = CC 
DEBUG = -O
#if HP, use this one
#CFLAGS = $(DEBUG) -I/usr/include/Motif1.2  -I../lib 
# else
CFLAGS = $(DEBUG) -I. -I../lib/ -I/usr/X11/include/

# If SUN, use this lib
#LIBS =  -L../lib -lXs -lXm -lXt -lX11 -lgen -lnsl -lsocket -lm
#if HP, use this
#LIBS =   -L../lib -lXs -L/usr/lib/Motif1.2 -L/usr/lib/X11R5  -lXm -lXt -lX11 -lPW  -lm
#if sgi use
LIBS =  -L/usr/X11/lib/ -L/usr/lib/ -lXm -lXt -lX11 -lc    

TARGETS = draw

all: $(TARGETS)

draw:  main.o graphics.o  help.o connectdialog.o addtextdialog.o MenuSupport.o wprintf.o parser.o editor.o mousetracker.o socket.o
	$(CC) -o $@ main.o graphics.o help.o connectdialog.o addtextdialog.o MenuSupport.o wprintf.o parser.o editor.o mousetracker.o socket.o $(LIBS)

clean:
	rm -f *.o *~* *.a 
	rm -f $(TARGETS)
	rm -f *.arg
	rm -f core
              

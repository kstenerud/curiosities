/*
snes.c

By Karl Stenerud (stenerud@lightspeed.bc.ca)

Original design by Benji York (benji@cookeville.com) author of sneskey.
See the sneskey page at: http://www.geocities.com/SiliconValley/Way/8843

This program is a simple test of the parallel SNES pad adator.

Obviously, you'll need to make a SNES pad adaptor to use it.
I've included the schematics for it here (use DOS extended character set):
(Taken from the SNESKey documentation)

******************************************************************************

         DB-25 (male)                 SNES Connector

        Pin 01  
       Pin 14  
        Pin 02 ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
       Pin 15  ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ³ÄÄÄÄÄÄÄÄÄ Data on 4th controll pad
        Pin 03 ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
       Pin 16                    ³  ³
        Pin 04                   ³  ³
       Pin 17                    ³  ³    ÚÄÄÄ¿
        Pin 05 ÄÄÄÄ>³ÄÄÄÄ¿   ÚÄÄÄÄÄÄÄÄÄÄÄ³ o ³  Vcc
    ÚÄ Pin 18            ³   ³   ÀÄÄÄÄÄÄÄ³ o ³  Clock
    ³   Pin 06 ÄÄÄÄ>³ÄÄÄÄ´   ³      ÀÄÄÄÄ³ o ³  Reset
    ÃÄ Pin 19            ³   ³  ÚÄÄÄÄÄÄÄÄ³ o ³  Data
    ³   Pin 07 ÄÄÄÄ>³ÄÄÄÄÅÄÄÄÙ  ³        ÃÄÄÄ´
    ÃÄ Pin 20            ³      ³        ³ o ³  N/C
    ³   Pin 08 ÄÄÄÄ>³ÄÄÄÄ´      ³        ³ o ³  N/C
    ÃÄ Pin 21            ³      ³   ÚÄÄÄÄ³ o ³  GND
    ³   Pin 09 ÄÄÄÄ>³ÄÄÄÄÙ      ³   ³     \_/ 
    ÃÄ Pin 22                   ³   ³
    ³   Pin 10 ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ   ³
    ÃÄ Pin 23                       ³
    ³   Pin 11                      ³
    ÃÄ Pin 24                       ³
    ³   Pin 12 ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ³ÄÄÄÄÄÄ Data on 2nd controll pad
    ÃÄ Pin 25                       ³
    ³   Pin 13 ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ³ÄÄÄÄÄÄ Data on 3rd controll pad
    ³                               ³
    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ


        The 2nd, 3rd, and 4th data are only for connecting up multiple
controll pads. Connect all the other pins exactly like the first pad, then
connect the data line of the 2nd, 3rd, and 4th SNES pads to the appropriate
pins on the db25. Configuring these controllers is discussed in the section
about the ini files.

        It would be best if the diodes are Germanium, because they use less
power, and since the power is being pulled from the data lines of the parallel
port, we don't want to use too much. If you use a regular controller this
arrangement will be ok, and provide a compact adapter for you SNES
controller, but if you want to get fancy (IR controllers, big joysticks,
programmable pads, multiple controllers on one port, etc.) you may wish to
pull power from the joystick port, or better yet, get a power pack that plugs
into the AC main and outputs +5V dc. This will allow you to use any controller
you could dream of (not to mention reduce the complexity of the converter from
simple to almost non-existent).

        The best way I have found to achieve a good result is to buy an
extension cable for your SNES controller. They usually are about $10 for a
pack of two. Carefully remove the part of the connector that plugs in to the
SNES console and cut off most of the pins crimped on to the wires. Get a male
DB-25 connector and solder what's left of the pins, diodes, and the DB-25
together. Try to make the total assembly as small as possible because you
want to be able to fit it inside the shell of the original connector. After
everything is connected and working, use a VERY strong glue to adhere the
DB-25 to the connector shell positioned in the open end. If you use the glue
well, what you end up with will be a very cool adapter cable. Of course, you
could do this to the end of a controller, but that seems a bit permanent to
me.

        One note: you need to connect the ground from any power adapter to
the ground created from pins 18 to 25 on the port, so everything will have a
common reference.

******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <dos.h>

#define SNES_PAD0    0x40	/* Pin 10 (nAck),      SP bit 6 */
#define SNES_PAD1    0x20	/* Pin 12 (Paper Out), SP bit 5 */
#define SNES_PAD2    0x10	/* Pin 13 (Select),    SP bit 4 */
#define SNES_PAD3    0x08	/* Pin 15 (nError),    SP bit 3 */
#define SNES_UP      0x0010
#define SNES_DOWN    0x0020
#define SNES_RIGHT   0x0080
#define SNES_LEFT    0x0040
#define SNES_START   0x0008
#define SNES_SELECT  0x0004
#define SNES_A       0x0100
#define SNES_B       0x0001
#define SNES_X       0x0200
#define SNES_Y       0x0002
#define SNES_L       0x0400
#define SNES_R       0x0800

void print_snes_status(short padstatus);
void read_snes_pads(short port, short pads[4]);

char erase_line[] = {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
                     0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};

int main(void)
{
   short port         = 0x0378;	/* assume LPT1.  You may want to change it */
   short now_pads[4];
   short last_pads[4] = {0xffff, 0xffff, 0xffff, 0xffff};
   int i;

   for(;;)
   {
      read_snes_pads(port, now_pads);
      if(now_pads[0] != last_pads[0] || now_pads[1] != last_pads[1] ||
         now_pads[2] != last_pads[2] || now_pads[3] != last_pads[3])
      {
         for(i=0;i<4;i++)
         {
            last_pads[i] = now_pads[i];
            print_snes_status(now_pads[i]);
         }
         write(1, erase_line, 56);
      }
   }
   return 0;
}

void print_snes_status(short padstatus)
{
   write(1, padstatus & SNES_UP ? "u" : ".", 1);
   write(1, padstatus & SNES_DOWN ? "d" : ".", 1);
   write(1, padstatus & SNES_LEFT ? "l" : ".", 1);
   write(1, padstatus & SNES_RIGHT ? "r" : ".", 1);
   write(1, padstatus & SNES_SELECT ? "s" : ".", 1);
   write(1, padstatus & SNES_START ? "S" : ".", 1);
   write(1, padstatus & SNES_A ? "A" : ".", 1);
   write(1, padstatus & SNES_B ? "B" : ".", 1);
   write(1, padstatus & SNES_X ? "X" : ".", 1);
   write(1, padstatus & SNES_Y ? "Y" : ".", 1);
   write(1, padstatus & SNES_L ? "L" : ".", 1);
   write(1, padstatus & SNES_R ? "R" : ".", 1);
   write(1, "  ", 2);
}

/* Read snes keypad status and return as bits in an array of 4 shorts
 * The values are placed, LSB to MSB, in the following order:
 * B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R
 */
void read_snes_pads(short port, short pads[4])
{
   int i;
   char result;

   pads[0] = pads[1] = pads[2] = pads[3] = 0;

   /* reset the snes pads */
                         /* VVVV V-RC */
   outportb(port, 0xfa); /* 1111 1010 */
   outportb(port, 0xf8); /* 1111 1000 */

   /* read in values from the SNES pad */
   for(i=0;i<12;i++)
   {
      /* read in the current button */
      result = inportb(port+1);
      pads[0] |= ((result & SNES_PAD0) == 0) << i;
      pads[1] |= ((result & SNES_PAD1) == 0) << i;
      pads[2] |= ((result & SNES_PAD2) == 0) << i;
      pads[3] |= ((result & SNES_PAD3) == 0) << i;

      /* send a clock tick */
                            /* VVVV V-RC */
      outportb(port, 0xf9); /* 1111 1001 */
      outportb(port, 0xf8); /* 1111 1000 */
   }
}

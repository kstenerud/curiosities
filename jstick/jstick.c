/* Joystick handler routines by Karl Stenerud (stenerud@lightspeed.bc.ca)
 *
 * These routines will handle up to 2 joysticks attached to the game port of a
 * PC.  It will provide digital and analog readouts of the joystick position.
 *
 * You may use this code in your game or whatever so long as you give me credit
 * and send me a copy of whatever you make.
 */


#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <mem.h>

/* Joystick directions (digital) */
#define JOY_LEFT	 1
#define JOY_RIGHT	 2
#define JOY_UP		 3
#define JOY_DOWN	 4
#define JOY_FIRE1	 5
#define JOY_FIRE2	 6
#define JOY2_LEFT	 7
#define JOY2_RIGHT	 8
#define JOY2_UP		 9
#define JOY2_DOWN	10
#define JOY2_FIRE1	11	/* NOTE: some joysticks take over 2nd jstick's */
#define JOY2_FIRE2	12	/* buttons to be used as 3rd and 4th buttons */
#define JOY_MAX		12

/* Joystick directions (analog) -- also used to detect presence of direction */
#define JOY_X		 1
#define JOY_Y		 2
#define JOY2_X		 3
#define JOY2_Y		 4

int init_joystick();
void poll_joystick();
int joystick_pressed(int direction);
int joystick_analog(int direction);
int joystick_direction_present(int direction);
const char* joystick_name(int direction);


/* internal use */
#define JS_UP    0x01
#define JS_DOWN  0x02
#define JS_LEFT  0x04
#define JS_RIGHT 0x08
#define JS_B1    0x10
#define JS_B2    0x20
#define JS_XPRES 0x40	/* means that X axis is presend */
#define JS_YPRES 0x80	/* means that Y axis is present */

/* 2 joysticks, 3 data values: x%, y%, flags */
unsigned short jstick[2][3] = {0};


/* Init Joystick routine by Karl Stenerud.
 * Tells us which joysticks and directions are present.
 * Direction checking is so that devices like pedals can be used.
 */
int init_joystick()
{
   unsigned short val[2][2] = {{0, 0}, {0, 0}};
   unsigned short limit = 0xffff;
   register unsigned char portval;

   jstick[0][0] = jstick[0][1] = jstick[1][0] = jstick[1][1] = 0;

   /* Start the timers */
   outportb(0x201, 0x00);

   /* Find out who changes within the time limit */
   while(limit--)
   {
      portval = inportb(0x201);
      if(portval & 0x01) val[0][0]++;
      if(portval & 0x02) val[0][1]++;
      if(portval & 0x04) val[1][0]++;
      if(portval & 0x08) val[1][1]++;
   }

   jstick[0][2] = ((val[0][0] != 0xFFFF && val[0][0] != 0) ? JS_XPRES : 0)
                | ((val[0][1] != 0xFFFF && val[0][1] != 0) ? JS_YPRES : 0);

   jstick[1][2] = ((val[1][0] != 0xFFFF && val[1][0] != 0) ? JS_XPRES : 0)
                | ((val[1][1] != 0xFFFF && val[1][1] != 0) ? JS_YPRES : 0);

   return (jstick[0][2] | jstick[1][2]) & (JS_XPRES | JS_YPRES);
}


/* Poll joystick routine by Karl Stenerud.
 * Values for the first joystick are stored in jstick[0][x] and values for
 * the second are in jstick[1][x].
 * jstick[x][0] contains the analog X value where 0 is far left, 50 is
 * centered, and 100 is far right.
 * jstick[x][1] contains the analog Y value where 0 is top, 50 is
 * centered, and 100 is bottom.
 * jstick[x][2] contains digital representations of left/right/up/down/b1/b2.
 */
void poll_joystick()
{
   static unsigned short mid[2][2] = {{0, 0}, {0, 0}};
   static unsigned short max[2][2] = {{0, 0}, {0, 0}};
   static short firstentry = 1;
   unsigned short limit = 0xffff;
   register unsigned char portval;
   int val_written;
   int i;

   /* Interrupts OFF */
   asm("cli");

   /* Start the timers */
   outportb(0x201, 0x00);


   jstick[0][0] = jstick[0][1] = jstick[1][0] = jstick[1][1] = 0;

   /* Wait for timers to run out */
   while (limit--)
   {
      portval = inportb(0x201);
      val_written = 0;

      if((jstick[0][2] & JS_XPRES) && (portval & 0x01)) {jstick[0][0]++;val_written=1;}
      if((jstick[0][2] & JS_YPRES) && (portval & 0x02)) {jstick[0][1]++;val_written=1;}
      if((jstick[1][2] & JS_XPRES) && (portval & 0x04)) {jstick[1][0]++;val_written=1;}
      if((jstick[1][2] & JS_YPRES) && (portval & 0x08)) {jstick[1][1]++;val_written=1;}

      if(!val_written)
         /* All timers have run out, so no need to poll anymore */
         break;
   }

   /* Set our reference values if this is the first call */
   if(firstentry)
   {
      firstentry = 0;

      /* We start by assuming max is 2 * mid.  We correct it later on */
      max[0][0] = (mid[0][0] = jstick[0][0])*2;
      max[0][1] = (mid[0][1] = jstick[0][1])*2;
      max[1][0] = (mid[1][0] = jstick[1][0])*2;
      max[1][1] = (mid[1][1] = jstick[1][1])*2;
   }

   /* Store joystick data */
   for(i=0;i<2;i++)
   {
      /* Store analog data as percent */
      if(jstick[i][2] & JS_XPRES)
      {
         if(jstick[i][0] <= mid[i][0])
            /* left or top to centered (0-50%) */
            jstick[i][0] = (50*jstick[i][0])/(mid[i][0]);
         else if(jstick[i][0] <= max[i][0])
            /* centered to right or bottom (50-100%) */
            jstick[i][0] = (50*(jstick[i][0]-mid[i][0]))/(max[i][0]-mid[i][0]) + 50;
         else
         {
            /* Auto-correct if value read is above max */
            max[i][0] = jstick[i][0];
            jstick[i][0] = 100;
         }
      }
      if(jstick[i][2] & JS_YPRES)
      {
         if(jstick[i][1] <= mid[i][1])
            jstick[i][1] = (50*jstick[i][1])/(mid[i][1]);
         else if(jstick[i][1] <= max[i][1])
            jstick[i][1] = (50*(jstick[i][1]-mid[i][1]))/(max[i][1]-mid[i][1]) + 50;
         else
         {
            max[i][1] = jstick[i][1];
            jstick[i][1] = 100;
         }
      }
      /* Store digital data */
      jstick[i][2] = (jstick[i][2] & (JS_XPRES | JS_YPRES))
                   | ((jstick[i][2] & JS_XPRES) && (jstick[i][0] < 25) ? JS_LEFT  : 0)
                   | ((jstick[i][2] & JS_XPRES) && (jstick[i][0] > 75) ? JS_RIGHT : 0)
                   | ((jstick[i][2] & JS_YPRES) && (jstick[i][1] < 25) ? JS_UP    : 0)
                   | ((jstick[i][2] & JS_YPRES) && (jstick[i][1] > 75) ? JS_DOWN  : 0)
                   | ((portval & (i == 0 ? 0x10 : 0x40)) ? 0 : JS_B1)
                   | ((portval & (i == 0 ? 0x20 : 0x80)) ? 0 : JS_B2);
   }

   /* Interrupts ON */
   asm("sti");
}


/* Check if a joystick direction is pressed.
 * Returns nonzero (TRUE) or zero (FALSE).
 */
int joystick_pressed(int direction)
{
   switch (direction)
   {
      case JOY_LEFT:
         return jstick[0][2] & JS_LEFT;
      case JOY_RIGHT:
         return jstick[0][2] & JS_RIGHT;
      case JOY_UP:
         return jstick[0][2] & JS_UP;
      case JOY_DOWN:
         return jstick[0][2] & JS_DOWN;
      case JOY_FIRE1:
         return jstick[0][2] & JS_B1;
      case JOY_FIRE2:
         return jstick[0][2] & JS_B2;
      case JOY2_LEFT:
         return jstick[1][2] & JS_LEFT;
      case JOY2_RIGHT:
         return jstick[1][2] & JS_RIGHT;
      case JOY2_UP:
         return jstick[1][2] & JS_UP;
      case JOY2_DOWN:
         return jstick[1][2] & JS_DOWN;
      case JOY2_FIRE1:
         return jstick[1][2] & JS_B1;
      case JOY2_FIRE2:
         return jstick[1][2] & JS_B2;
   }
   return 0;
}


/* Get the analog values on the joystick.
 * For X: 0 = all the way left, 100 = all the way right.
 * For Y: 0 = all the way up, 100 = all the way down.
 */
int joystick_analog(int direction)
{
   switch (direction)
   {
      case JOY_X:
        return jstick[0][0];
      case JOY_Y:
        return jstick[0][1];
      case JOY2_X:
        return jstick[1][0];
      case JOY2_Y:
        return jstick[1][1];
   }
   return 0;
}


/* Tells if a joystick direction is present on this joystick.
 * Returns nonzero (TRUE) or zero (FALSE).
 */
int joystick_direction_present(int direction)
{
   switch (direction)
   {
      case JOY_X:
        return jstick[0][2] & JS_XPRES;
      case JOY_Y:
        return jstick[0][2] & JS_YPRES;
      case JOY2_X:
        return jstick[1][2] & JS_XPRES;
      case JOY2_Y:
        return jstick[1][2] & JS_YPRES;
   }
   return 0;
}


/* Get the name of a joystick direction/button */
const char* joystick_name(int direction)
{
  char* names[] = { "None",
                    "J1 LEFT", "J1 RIGHT", "J1 UP", "J1 DOWN",
                    "J1 B1", "J1 B2", "J2 LEFT", "J2 RIGHT",
                    "J2 UP", "J2 DOWN", "J2 B1", "J2 B2"};

  if(direction > 0 && direction <= JOY_MAX)
     return names[direction];
  return names[0];
}


int main(void)
{
   char buff[100];
   int i;

   unsigned short old_jstick[2][3] = {0};

   if(!init_joystick())
   {
      gotoxy(1, 22);
      cprintf("NO JOYSTICK!!!");
   }

   for(;;)
   {
      poll_joystick();
      if(memcmp(old_jstick, jstick, sizeof(unsigned short)*6) != 0)
      {
         memcpy(old_jstick, jstick, sizeof(unsigned short)*6);
         sprintf(buff, "A: %c%c%c%c%c%c%c%c %3dx %3dy,  B:%c%c%c%c%c%c%c%c %3dx %3dy",
                 joystick_pressed(JOY_UP)           ? 'u' : '.',
                 joystick_pressed(JOY_DOWN)         ? 'd' : '.',
                 joystick_pressed(JOY_LEFT)         ? 'l' : '.',
                 joystick_pressed(JOY_RIGHT)        ? 'r' : '.',
                 joystick_pressed(JOY_FIRE1)        ? '1' : '.',
                 joystick_pressed(JOY_FIRE2)        ? '2' : '.',
                 joystick_direction_present(JOY_X)  ? 'X' : '.',
                 joystick_direction_present(JOY_Y)  ? 'Y' : '.',
                 joystick_analog(JOY_X),
                 joystick_analog(JOY_Y),
                 joystick_pressed(JOY2_UP)          ? 'u' : '.',
                 joystick_pressed(JOY2_DOWN)        ? 'd' : '.',
                 joystick_pressed(JOY2_LEFT)        ? 'l' : '.',
                 joystick_pressed(JOY2_RIGHT)       ? 'r' : '.',
                 joystick_pressed(JOY2_FIRE1)       ? '1' : '.',
                 joystick_pressed(JOY2_FIRE2)       ? '2' : '.',
                 joystick_direction_present(JOY2_X) ? 'X' : '.',
                 joystick_direction_present(JOY2_Y) ? 'Y' : '.',
                 joystick_analog(JOY2_X),
                 joystick_analog(JOY2_Y)
                );
         gotoxy(1, 24);
         write(1, buff, strlen(buff));
         gotoxy(1, 23);
         for(i=1;i<=JOY_MAX;i++)
         {
            if(joystick_pressed(i))
            {
               cprintf("%s     ", joystick_name(i));
               break;
            }
         }
      }
   }
   return 0;
}

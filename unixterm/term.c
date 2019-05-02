#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include "terminal.h"

void catch_c(int nothing);		// handle interrupt
pid_t pid;				// child's pid
Terminal term;				// handles console terminal
Terminal modem;				// handles modem

int main(int argc, char* argv[])
{
   int  baud   = 19200;			// default settings
   char data   = 8;
   char stop   = 1;
   char parity = 'n';
   int i;				// counter
   char buff[1024];			// basic buffer
   int len;				// basic length counter
   char ch;

   // handle command line arguments
   if(argc < 2)
   {
      printf("Useage: term <device> [OPTIONS]\n");
      printf("	-b <baud>	Baud rate (default is 19200)\n");
      printf("	-d <data>	Data bits (valid range is 5-8, default 8)\n");
      printf("	-p <parity>	Parity (o=odd, e=even, n=none, default n)\n");
      printf("	-s <stop>	Stop bits (valid range is 1-2, default 1)\n");
      exit(1);
   }


   // check for switches
   for(i=2; i<argc; i++)
   {
      if(( argv[i][0] != '-') || (strlen(argv[i]) != 2))
      {
         printf("%s: invalid option\n", argv[i]);
         exit(1);
      }

      // valid switch?
      switch(argv[i][1])
      {
      case 'b':
         if( (i+1) >= argc )
         {
            printf("%s: missing argument\n", argv[i]);
            exit(1);
         }
         baud = atoi(argv[++i]);
         if(baud != 50 && baud != 75 && baud != 110 & baud != 134 && baud != 150
            && baud != 200 && baud != 300 & baud != 600 & baud != 1200
            && baud != 2400 && baud != 4800 & baud != 9600 & baud != 19200
            && baud != 38400 && baud != 57600 && baud != 115200 & baud != 230400
            && baud != 460800)
         {
            printf("Invalid baud rate.  Valid rates are:\n");
            printf("50, 75, 110, 134, 150, 200, 300, 600, 1200, 2400, 4800,\n");
            printf("9600, 19200, 38400, 57600, 115200, 230400, 460800\n");
            exit(1);
         }
         break;
      case 'd':
         if( (i+1) >= argc )
         {
            printf("%s: missing argument\n", argv[i]);
            exit(1);
         }
         data = atoi(argv[++i]);
         if(data < 5 || data > 8)
         {
            printf("Invalid data bits.  Valid data bits are: 5, 6, 7, 8\n");
            exit(1);
         }
         break;
      case 'p':
         if( (i+1) >= argc )
         {
            printf("%s: missing argument\n", argv[i]);
            exit(1);
         }
         i++;
         if(argv[i][0] != 'e' && argv[i][0] != 'o' && argv[i][0] != 'n')
         {
            printf("Invalid parity. Valid parity settings are: e, o, n\n");
            exit(1);
         }
         parity = argv[i][0];
         break;
      case 's':
         if( (i+1) >= argc )
         {
            printf("%s: missing argument\n", argv[i]);
            exit(1);
         }
         stop = atoi(argv[++i]);
         if(stop < 1 || stop > 2)
         {
            printf("Invalid stop bits.  Valid stop bits are: 1, 2\n");
            exit(1);
         }
         break;
      default:
         printf("%s: invalid option\n", argv[i]);
         exit(1);
      }
   }

   // try to open the modem
   if(modem.open(argv[1], baud, data, parity, stop, 1) < 0)
   {
      printf("Couldn't open device %s\n", argv[1]);
      exit(1);
   }

   system("clear");

   // manage the console terminal
   term.manage_terminal(1, 1);

   printf("Opened device %s with settings %d %d%c%d.  Press ^/ for command mode.\n"
          , argv[1], baud, data, parity, stop);

   signal(SIGINT, catch_c);

   // now our processes part ways
   switch(pid=fork())
   {
   case -1:
      perror("main: fork");
      exit(1);
   case 0:
      // child reads the modem and writes to console
      for(;;)
      {
         if( (len=modem.wait_and_read(buff, 1024, -1)) < 0 )
            exit(0);
         if(len > 0)
            term.write(buff, len);
      }
   default:
      // parent reads from console and writes to modem
      for(;;)
      {
         ch = term.wait_and_readchar(-1);
         if(ch == 0x1f)
         {
            term.writestring("\n[h]angup, [q]uit, or [c]ontinue : ");
            ch = term.wait_and_readchar(-1);
            term.writechar('\n');

            if(ch == 'h' || ch == 'H')
            {
               modem.hangup();
            }
            else if(ch == 'q' || ch == 'Q')
               catch_c(0);
         }
         else
            modem.writechar(ch);
      }
   }

   return 0;
}


//------------------------------------------------------------------------------
// name:   catch_c
// desc:   catches SIGINT and restores our terminal
// Inputs: nothing - completely ignored
// Output: none
void catch_c(int nothing)
{
   term.restore_settings();		// restore our terminal
   modem.restore_settings();		// restore modem, too
   if(pid > 0)				// interrupt child pid if necessary
   {
      kill(pid, SIGINT);
      wait(NULL);
   }
   exit(0);
}

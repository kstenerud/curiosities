#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include "socklist.h"
#include "player.h"
#include "stock.h"
#include "textstr.h"
#include "timer.h"
#include "cmds.h"

// g++ didn't like static members in a class, and so I couldn't do a singleton
// implementation.  The only other alternative is to use globals.
extern PlayerList Plist;
extern StockList Slist;

void roll_stocks(int);			// roll the stocks
int roll(int, int);			// 1 roll
int rand_5_10_20();			// random 5, 10, or 20 value
void reset_pfiles();			// reset player files


int main(void)
{
   Socket_list slist(9000, 3);		// port 9000, max 3 connects
   Socket*     temp_sock;		// temp sock for "for" loops
   char        buff[100];		// buffer for writing to stdout
   Timer       timer(30);		// event timer
   Cmd*        cmd;			// command to execute
   int         quit_prg = 0;		// quit program (true/false)
   Textstring  user_input;		// input from user
   Player*     player;			// holds player objects
   int         max_time = 7*24*60*60*2; // 1 week in 30 second units
   int         time_left = max_time;	// time left in this game

   signal( SIGPIPE, SIG_IGN );		// ignore broken pipes

   srand( (unsigned int) time(NULL) );
   timer.reset();

   while(!quit_prg)
   {
      // Wait for input from the sockets for however much time is left on the
      // event timer.
      slist.wait_for_input(timer.left());

      // check for new connections
      if( (temp_sock=slist.add_new_connection()) != NULL)
         Plist.add(new Player(temp_sock));

      slist.gather_input();	// buffer input on connections

      // check if there are commands to execute
      for(temp_sock = slist.first_input();temp_sock != NULL
          ;temp_sock = slist.next_input())
      {
         // get the command
         user_input = temp_sock->get_current_cmd();

         // clear the command buffer
         temp_sock->clear_current_cmd();

         if( (player=temp_sock->get_player()) == NULL )
            continue;

         cmd = get_cmd(player, user_input);

         switch(cmd->execute())
         {
         case 1:        // quit
            slist.remove(temp_sock);
            break;
         case 2:        // shutdown
            Plist.save_all();
            quit_prg = 1;
            break;
         }
         delete cmd;
      }

      // check our event timer.
      if(timer.isset())
      {
         if(--time_left <=0)	// game finished. declare winner and reset
         {
            Plist.declare_winner();
            Plist.reset_players();
            Slist.reset_stocks();
            reset_pfiles();
            Plist.msg_all("SAY Server: new game.\n");
            Plist.stat_all();
            time_left = max_time;
         }
         else
         {
            if((time_left % 2) == 0)
            {
               sprintf(buff, "TIME %d\n", time_left/2);
               Plist.msg_all(buff);
            }

            timer.clear();
            roll_stocks(6);
            Plist.stat_all();
         }
      }
   }
   return 0;				// adios
}

//------------------------------------------------------------------------------
// name:   roll_stocks
// desc:   use a random generator to either increase stock values, decrease
//         them, or declare a dividend.
// inputs: num_rolls - number of rolls to make
// output: void
void roll_stocks(int num_rolls)
{
   int stockno;

   for(int i=0;i<num_rolls;i++)
   {
      stockno = roll(0, 5);
      switch(roll(1, 3))
      {
      case 1:		// up
         Slist.get_stock(stockno)->modify_value(rand_5_10_20());
         break;
      case 2:		// down
         Slist.get_stock(stockno)->modify_value(rand_5_10_20()*-1);
         break;
      case 3:		// dividend
         if(Slist.get_stock(stockno)->get_value() >= 100)
            Plist.dividend_all(stockno, rand_5_10_20());
         break;
      default:
         write(2, "Bad roll\n", 9);
      }
   }
}

//------------------------------------------------------------------------------
// name:   roll
// desc:   generate a random integer
// inputs: min - minimim value to return
//         max - maximum value to return
// output: value generated
int roll(int min, int max)
{
  return min+(int) ((float)max*rand()/(RAND_MAX+1.0));
}

//------------------------------------------------------------------------------
// name:   rand_5_10_20
// desc:   generate a random value of 5, 10, or 20
// inputs: void
// output: value generated
int rand_5_10_20()
{
   switch(roll(1, 3))
   {
   case 1:
      return 5;
   case 2:
      return 10;
   case 3:
      return 20;
   default:
      write(2, "Bad roll\n", 9);
   }
}

//------------------------------------------------------------------------------
// name:   reset_pfiles
// desc:   erase all the player files, but save the admin file
// inputs: void
// output: void
void reset_pfiles()
{
   system("mv -f players/admin admin/.");
   system("rm -f players/*");
   system("mv -f admin/admin players/.");
}

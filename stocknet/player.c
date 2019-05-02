#include "player.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

PlayerList Plist;
StockList Slist;

//------------------------------------------------------------------------------
// name:   create_id
// desc:   create a unique 32-bit integer id
// inputs: void
// output: unique id
int create_id()
{
   static int id = 1;

   return ++id;
}

//==============================================================================

Player::Player(Socket* sock)
: _name(""), _cash(500000), _sock(sock), _status(askname), _id(::create_id())
, _loan(0), _admin(0)
{
   for(int i=0;i<6;i++)
   {
      _stocks[i] = Slist.get_stock(i);	// get the active stocks
      _stock_quantity[i] = 0;		// we have no stocks yet
   }
   _sock = sock;
   _sock->attach_player(_id);		// attach this ID to the socket
   _sock->writestring("NAME\n");	// prompt for user name
                                        // (our initial state is askname)
}

//------------------------------------------------------------------------------
// name:   Player::stock_crash
// desc:   Take care of a stock crash from the player's side
// inputs: stockno - the stock that crashed
// output: void
void Player::stock_crash(int stockno)
{
   char str[20];

   sprintf(str, "CRASH %d\n", stockno);	// notify user
   message(str);
   _stock_quantity[stockno] = 0;	// remove all the stocks of this type
}

//------------------------------------------------------------------------------
// name:   Player::stock_split
// desc:   Take care of a stock split from the player's side
// inputs: stockno - the stock that split
// output: void
void Player::stock_split(int stockno)
{
   char str[20];

   sprintf(str, "SPLIT %d\n", stockno);	// notify user
   message(str);
   _stock_quantity[stockno] *= 2;	// double the stocks of this type
}

//------------------------------------------------------------------------------
// name:   Player::loan()
// desc:   Try to get a loan for this player.
// inputs: void
// output: void
void Player::loan()
{
   if(_loan)
   {
      error("You already have a loan!\n");
      return;
   }
   if(_cash > 0)
   {
      error("You already have money!\n");
      return;
   }
   if(!(_stock_quantity[0] == 0 && _stock_quantity[1] == 0 &&
        _stock_quantity[2] == 0 && _stock_quantity[3] == 0 &&
        _stock_quantity[4] == 0 && _stock_quantity[5] == 0 ))
   {
      error("Sell your stocks first.\n");
      return;
   }

   // cool, we got the loan.
   _cash = 1000;
   _loan = 1;
   stat();
}

//------------------------------------------------------------------------------
// name:   Player::who_entry
// desc:   Send one WHO entry to the requesting player
// inputs: dest - the player who wants to know
// output: void
void Player::who_entry(Player* dest)
{
   char str[200];

   if(_status != playing)	// don't include people logging in
      return;

   sprintf(str, "WHO %15s %10d %6d %6d %6d %6d %6d %6d\n", _name.get_string(),
           _cash-(_loan ? 2000 : 0),
           _stock_quantity[0], _stock_quantity[1], _stock_quantity[2],
           _stock_quantity[3], _stock_quantity[4], _stock_quantity[5]);
   dest->message(str);
}


//------------------------------------------------------------------------------
// name:   Player::stat
// desc:   Send the player's current status
// inputs: void
// output: void
void Player::stat()
{
   char str[1024];

   if(_status != playing)	// not if they're logging in
      return;

   // send cash, all the stock values, and the amount of each stock we have
   sprintf(str, "STAT %d %d %d %d %d %d %d %d %d %d %d %d %d\n", _cash,
           _stocks[0]->get_value(), _stocks[1]->get_value(),
           _stocks[2]->get_value(), _stocks[3]->get_value(),
           _stocks[4]->get_value(), _stocks[5]->get_value(),
           _stock_quantity[0], _stock_quantity[1], _stock_quantity[2],
           _stock_quantity[3], _stock_quantity[4], _stock_quantity[5] );
   message(str);
}

//------------------------------------------------------------------------------
// name:   Player::receive_dividend
// desc:   handle a dividend from the player's side
// inputs: stockno - the stock that generated a dividend
//         percent - what percentage we get
// output: TRUE/FALSE (success/failure)
int Player::receive_dividend(int stockno, int percent)
{
   int amount = 0;
   char buff[200];

   if(this==NULL ||stockno < 0 || stockno >= 6 || percent < 1 || percent > 100)
      return 1==0;

   if(_status != playing)
      return 1==0;

   // We deal in cents, so no need to divide by 100
   if(_stocks[stockno]->get_value() >= 100)
      amount = _stock_quantity[stockno] * percent;
   _cash += amount;

   // now inform the user
   sprintf(buff, "DIV %d %d %d\n", stockno, percent, amount);
   message(buff);
   return 1==1;
}

//------------------------------------------------------------------------------
// name:   Player::buy
// desc:   Buy some stocks.
// inputs: stockno - the stock to buy
//         quantity - how many to buy
//         unit_price - price per unit
// output: TRUE/FALSE (success/fail)
int Player::buy(int stockno, int quantity, int unit_price)
{
   int price;

   if(this == NULL || stockno < 0 || stockno >= 6)
      return 1==0;

   price = unit_price * quantity;
   if(price <= 0 || price > _cash)		// do we have wnough cash?
      return 1==0;

   _cash -= price;				// buy it then
   _stock_quantity[stockno] += quantity;
   return 1==1;
}

//------------------------------------------------------------------------------
// name:   Player::sell
// desc:   Buy some stocks.
// inputs: stockno - the stock to buy
//         quantity - how many to buy
//         unit_price - price per unit
// output: TRUE/FALSE (success/fail)
int Player::sell(int stockno, int quantity, int unit_price)
{
   int price;

   if(this == NULL)
      return 1==0;

   if(stockno < 0 || stockno >= 6 || quantity > _stock_quantity[stockno])
      return 1==0;
				// do we have enough to sell?

   price = unit_price * quantity;
   if(price <= 0)
      return 1==0;

   _cash += price;		// sell it then
   _stock_quantity[stockno] -= quantity;
   return 1==1;
}

//------------------------------------------------------------------------------
// name:   Player::cashout()
// desc:   Sell all our stocks
// inputs: void
// output: void
void Player::cashout()
{
   if(this == NULL)
      return;
   if(_status != playing)
      return;

   for(int i=0;i<6;i++)			// get all stock and sell
      sell(i, _stock_quantity[i], _stocks[i]->get_value());
}

//------------------------------------------------------------------------------
// name:   Player::quit()
// desc:   Quit the game
// inputs: void
// output: void
void Player::quit()
{
   if(this == NULL)
      return;
   if(_status == playing)
   {
      cashout();		// cashout and save first
      save();
   }
   Plist.remove(this);		// sayonara!
}

//------------------------------------------------------------------------------
// name:   Player::load()
// desc:   Try to load player data
// inputs: void
// output: TRUE/FALSE (success/fail)
int Player::load()
{
   char buff[100];
   char* ptr = buff;
   int fd;
   int len;

   // load file from players directory
   sprintf(buff, "players/%s", _name.get_string());

   if( (fd=open(buff, O_RDONLY)) < 0)	// try to load the file
      return 1==0;

   if( (len=read(fd, buff, 100)) < 0)
   {
      perror("Player::load: read");
      exit(1);
   }

   _passwd = ptr;		// password
   ptr += strlen(ptr)+1;
   _cash = atoi(ptr);		// cash
   ptr += strlen(ptr)+1;
   _loan = atoi(ptr);		// loan status
   ptr += strlen(ptr)+1;
   _admin = atoi(ptr);		// administrator bit

   close(fd);
   return 1==1;
}

//------------------------------------------------------------------------------
// name:   Player::load()
// desc:   Try to load player data
// inputs: void
// output: TRUE/FALSE (success/fail)
int Player::save()
{
   char buff[100];
   int fd;
   int len;

   sprintf(buff, "players/%s", _name.get_string());

   if( (fd=open(buff, O_WRONLY|O_CREAT)) < 0)	// try to create the file
   {
      perror("Player::save: open");
      exit(1);
   }

   strcpy(buff, _passwd.get_string());		// save stuff
   write(fd, buff, strlen(buff)+1);
   sprintf(buff, "%d", _cash);
   write(fd, buff, strlen(buff)+1);
   sprintf(buff, "%d", _loan);
   write(fd, buff, strlen(buff)+1);
   sprintf(buff, "%d", _admin);
   write(fd, buff, strlen(buff)+1);

   close(fd);
   return 1==1;
}


//==============================================================================


//------------------------------------------------------------------------------
// name:   PlayerList::find
// desc:   Try to find a player by name
// inputs: name - the name to search for
// output: Player* - pointer to player object or NULL
Player* PlayerList::find(Textstring name)
{
   if(this == NULL)
      return NULL;

   for(int i=0;i<_len;i++)		// search the whole list
      if(_list[i] != NULL)
         if(_list[i]->get_name() == name)
            return _list[i];
   return NULL;
}

//------------------------------------------------------------------------------
// name:   PlayerList::find
// desc:   Try to find a player by id
// inputs: id - the id to search for
// output: Player* - pointer to player object or NULL
Player* PlayerList::find(int id)
{
   if(this == NULL)
      return NULL;

   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         if(_list[i]->get_id() == id)
            return _list[i];
   return NULL;
}

//------------------------------------------------------------------------------
// name:   PlayerList::add
// desc:   Add a player to the list
// inputs: player - the player to add
// output: TRUE/FALSE (success/fail)
int PlayerList::add(Player* player)
{
   if(this == NULL || player == NULL)
      return 1==0;

   for(int i=0;i<_len;i++)
      if(_list[i] == NULL)
      {
         _list[i] = player;
         return 1==1;
      }
   return 1==0;
}

//------------------------------------------------------------------------------
// name:   PlayerList::remove
// desc:   remove a player from the list
// inputs: player - the player to remove
// output: TRUE/FALSE (success/fail)
int PlayerList::remove(Player* player)
{
   if(this == NULL || player == NULL)
      return 1==0;

   for(int i=0;i<_len;i++)
      if(_list[i] == player)
      {
         _list[i] = NULL;
         delete player;
         return 1==1;
      }
   return 1==0;
}

//------------------------------------------------------------------------------
// name:   PlayerList::save_all
// desc:   send a save message to all players on the list
// inputs: void
// output: void
void PlayerList::save_all()
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
      {
         _list[i]->cashout();
         _list[i]->save();
      }
}

//------------------------------------------------------------------------------
// name:   PlayerList::reset_players
// desc:   force all players to cashout and reset their cash to initial value
// inputs: void
// output: void
void PlayerList::reset_players()
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
      {
         _list[i]->cashout();
         _list[i]->set_cash(500000);
      }
}

//------------------------------------------------------------------------------
// name:   PlayerList::declare_winner
// desc:   find the player with the highest net worth and tell everyone
// inputs: void
// output: void
void PlayerList::declare_winner()
{
   Player* winner = NULL;		// holds the winner's player object
   int money = 0;			// how much they were worth
   char buff[200];			// temp buffer

   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
      {
         _list[i]->cashout();		// have 'em cash out first
         // won't detect ties, but there's not enough time to code it.
         if(_list[i]->get_cash() > money) // do they have more money?
         {
            winner = _list[i];
            money = winner->get_cash();
         }
      }
   if(winner == NULL)			// no one playing, oh well.
      return;

   sprintf(buff, "SAY Server: winner is %s with $%d.%2d\n",
           winner->get_name().get_string(), winner->get_cash() / 100,
           winner->get_cash() % 100);
   msg_all(buff);
}

//------------------------------------------------------------------------------
// name:   PlayerList::stock_split
// desc:   forward a stock_split message to all active players
// inputs: stockno - the stock that split
// output: void
void PlayerList::stock_split(int stockno)
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         _list[i]->stock_split(stockno);
}

//------------------------------------------------------------------------------
// name:   PlayerList::stock_crash
// desc:   forward a stock_crash message to all active players
// inputs: stockno - the stock that crashed
// output: void
void PlayerList::stock_crash(int stockno)
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         _list[i]->stock_crash(stockno);
}

//------------------------------------------------------------------------------
// name:   PlayerList::stock_split
// desc:   forward a devidend message to all active players
// inputs: stockno - the stock that generated a dividend
//         percent - what percent dividend it generated
// output: void
void PlayerList::dividend_all(int stockno, int percent)
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         _list[i]->receive_dividend(stockno, percent);
}

//------------------------------------------------------------------------------
// name:   PlayerList::fame_list
// desc:   send a "hall of fame" list to the player
// inputs: to - the player who requested it
// output: void
void PlayerList::fame_list(Player* to)
{
   to->error("Not implemented.\n");
}

//------------------------------------------------------------------------------
// name:   PlayerList::stat_all
// desc:   forward a stat message to all active players
// inputs: void
// output: void
void PlayerList::stat_all()
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         _list[i]->stat();
}

//------------------------------------------------------------------------------
// name:   PlayerList::Who_List
// desc:   generate a list of players online
// inputs: to - the player who requested it
// output: void
void PlayerList::Who_List(Player* to)
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         _list[i]->who_entry(to);
}

//------------------------------------------------------------------------------
// name:   PlayerList::msg_all
// desc:   forward a message to all active players
// inputs: message - the mesage to forward
// output: void
void PlayerList::msg_all(Textstring message)
{
   for(int i=0;i<_len;i++)
      if(_list[i] != NULL)
         _list[i]->message(message);
}

//------------------------------------------------------------------------------
// name:   PlayerList::size
// desc:   calculate how many players are in the list
// inputs: void
// output: int - the number of players currently connected
int PlayerList::size()
{
   int i=0;
   int j=0;

   for(;i<_len;i++)
      if(_list[i] != NULL)
         j++;
   return j;
}

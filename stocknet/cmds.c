#include "cmds.h"
#include "player.h"

extern PlayerList Plist;

// an array of input parser functions, used depending on the current player
// state.
Cmd* (*cmd_parsers[])(Player*, Textstring&) = {	get_cmd_askname,
						get_cmd_askpass,
						get_cmd_asknew,
						get_cmd_newpass,
						get_cmd_playing,
					      };

//------------------------------------------------------------------------------
// name:   get_cmd
// desc:   get a command object to be executed.  This function actually
//         just calls the appropriate function to get the command, depending
//         on the player state.
// inputs: player - the player who input the command
//         string - the command itself
// output: Cmd* - the command to execute
Cmd* get_cmd(Player* player, Textstring& string)
{
   return (*cmd_parsers[player->get_status()])(player, string);
}


//------------------------------------------------------------------------------
// name:   get_cmd_askname
// desc:   get a command object to be executed.
//         This function will just take the input as the user name and
//         create a command that will try to load that player.
// inputs: player - the player who input the command
//         string - the user name
// output: Cmd* - the command to execute
Cmd* get_cmd_askname(Player* plr, Textstring& string)
{
   return new Cmd_Load_Player(plr, string);
}

//------------------------------------------------------------------------------
// name:   Cmd_Load_Player::execute
// desc:   Try to load a player.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Load_Player::execute()
{
   if(_name.get_length() > 15)
   {
      _player->error("Name too long (max 15).\n");
      return 1;
   }
   if(_name.has_nonalpha_chars())
   {
      _player->error("Name cannot have special characters.\n");
      return 1;
   }
   _player->set_name(_name);		// name is ok
   if(!_player->load())			// now try to load
   {
      _player->message("NEW?\n");	// can't load. ask if new
      _player->set_status(asknew);	// set player status accordingly
      return 0;
   }
   _player->message("PASS\n");		// ask for password
   _player->set_status(askpass);	// set player status accordingly
   return 0;
}

//------------------------------------------------------------------------------
// name:   get_cmd_askname
// desc:   get a command object to be executed.
//         This function will just take the input as the user name and
//         create a command that will try to confirm the player's password.
// inputs: player - the player who input the command
//         string - the password
// output: Cmd* - the command to execute
Cmd* get_cmd_askpass(Player* plr, Textstring& string)
{
   return new Cmd_Check_Passwd(plr, string);
}

//------------------------------------------------------------------------------
// name:   Cmd_Check_Passwd::execute
// desc:   Try to confirm the player's password.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Check_Passwd::execute()
{
   if(!_player->is_passwd(_pass))
   {
      _player->error("Invalid password.\n");
      return 1;
   }
   _player->set_status(playing);	// password ok.  set status to playing.
   _player->stat();			// send an initial STAT line
   return 0;
}

//------------------------------------------------------------------------------
// name:   get_cmd_asknew
// desc:   get a command object to be executed.
//         This function will just take the input as the user name and
//         create a command that will try to confirm if the player is new.
// inputs: player - the player who input the command
//         string - "y", "Y", or something else.
// output: Cmd* - the command to execute
Cmd* get_cmd_asknew(Player* plr, Textstring& string)
{
   return new Cmd_New_YN(plr, string);
}

//------------------------------------------------------------------------------
// name:   Cmd_New_YN::execute
// desc:   Try to confirm if the player is new.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_New_YN::execute()
{
   if(_reply != "y" && _reply != "Y")
   {
      return 1;		// says he wasn't a new player, but we have no pfile.
   }

   // ok so he is a new player.
   _player->message("PASS\n");		// prompt for a password
   _player->set_status(newpass);	// set status accordingly
   return 0;
}

//------------------------------------------------------------------------------
// name:   get_cmd_newpass
// desc:   get a command object to be executed.
//         This function will just take the input as the user name and
//         create a command that will get a new player's password.
// inputs: player - the player who input the command
//         string - the password
// output: Cmd* - the command to execute
Cmd* get_cmd_newpass(Player* plr, Textstring& string)
{
   return new Cmd_New_Pass(plr, string);
}

//------------------------------------------------------------------------------
// name:   Cmd_New_Pass::execute
// desc:   Try to confirm if the player is new.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_New_Pass::execute()
{
   if(_pass.get_length() > 15)
   {
      _player->error("Password too long (max 15).\n");
      return 1;
   }
   _player->set_passwd(_pass);		// get the password
   _player->message("PAS2\n");		// prompt to confirm password
   _player->set_status(askpass);	// set status accordingly
   return 0;
}



//------------------------------------------------------------------------------
// name:   get_cmd_playing
// desc:   get a command object to be executed.
//         This function is the one that parses actual commands.
// inputs: player - the player who input the command
//         string - the command
// output: Cmd* - the command to execute
Cmd* get_cmd_playing(Player* plr, Textstring& string)
{
   Textstring cmd  = string.extract_argument();	// the command
   Textstring arg1;				// arguments
   Textstring arg2;
   Textstring arg3;


   if(cmd == "QUIT")				// quit
      return new Cmd_Quit(plr);
   else if(cmd == "BUY")			// buy some stock
   {
      arg1 = string.extract_argument();		// what stock
      arg2 = string.extract_argument();		// how many
      arg3 = string.extract_argument();		// at what price
      return new Cmd_Buy(plr, arg1, arg2, arg3);
   }
   else if(cmd == "SELL")			// sell some stock
   {
      arg1 = string.extract_argument();		// what stock
      arg2 = string.extract_argument();		// how many
      arg3 = string.extract_argument();		// at what price
      return new Cmd_Sell(plr, arg1, arg2, arg3);
   }
   else if(cmd == "STAT")			// request status line
      return new Cmd_Stat(plr);
   else if(cmd == "SAY")			// say something to everyone
      return new Cmd_Say(plr, string);
   else if(cmd == "QWHO")			// who is online
      return new Cmd_Who(plr);
   else if(cmd == "QFAM")			// hall of fame
      return new Cmd_Fame(plr);
   else if(cmd == "SHUTDOWN")			// shutdown the game
      return new Cmd_Shutdown(plr, string);
   else if(cmd == "BANK")			// sell all stocks
      return new Cmd_Bank(plr);
   else if(cmd == "LOAN")			// ask for a loan
      return new Cmd_Loan(plr);
   else
      return new Cmd_Unknown(plr, cmd);		// unknown command
}

//------------------------------------------------------------------------------
// name:   Cmd_Loan::execute
// desc:   Try to get a loan for this player.  We just pass the buck to
//         Player::loan().
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Loan::execute()
{
   _player->loan();
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Shutdown::execute
// desc:   Shutdown the game.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Shutdown::execute()
{
   char str[200];

   if(!_player->is_admin())
   {
      _player->error("You are not an admin.\n");
      return 0;
   }

   sprintf(str, "QUIT %s\n", _reason.get_string());

   Plist.msg_all(str);
   return 2;
}

//------------------------------------------------------------------------------
// name:   Cmd_Fame::execute
// desc:   Ask class PlayerList to generate a hall of fame for us.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Fame::execute()
{
   Plist.fame_list(_player);
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Who::execute
// desc:   Ask class PlayerList to generate a list of players for us.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Who::execute()
{
   _player->message("WHOC\n");	// send a clearscreen message
   Plist.Who_List(_player);
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Say::execute
// desc:   Say something to all the other players.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Say::execute()
{
   char str[1000];

   if(_msg.get_length() > 900)
   {
      _player->error("Line too long.\n");
      return 0;
   }

   sprintf(str, "SAY %s: %s\n", _player->get_name().get_string(),
           _msg.get_string());

   Plist.msg_all(str);
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Stat::execute
// desc:   Ask class Player to generate a STAT line.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Stat::execute()
{
   _player->stat();
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Buy::execute
// desc:   Attempt to buy some stock.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Buy::execute()
{
   int stockno  = _arg1.get_number();
   int quantity = _arg2.get_number();
   int price    = _arg3.get_number();
   if(stockno < 0 || stockno >=6)
      _player->error("invalid stock number.\n");
   else if(quantity < 1 || price < 1)
      _player->error("invalid BUY syntax.\n");
   else if(!_player->buy(stockno, quantity, price))	// try to buy
      _player->error("You can't buy that many.\n");
   _player->stat();					// send STAT line
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Sell::execute
// desc:   Attempt to sell some stock.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Sell::execute()
{
   int stockno  = _arg1.get_number();
   int quantity = _arg2.get_number();
   int price    = _arg3.get_number();

   if(stockno < 0 || stockno >=6)
      _player->error("invalid stock number.\n");
   else if(quantity < 1 || price < 1)
      _player->error("invalid SELL syntax.\n");
   else if(!_player->sell(stockno, quantity, price))	// try to sell
      _player->error("You can't sell that many.\n");
   _player->stat();					// send STAT line
   return 0;
}

//------------------------------------------------------------------------------
// name:   Cmd_Unknown::execute
// desc:   Tell player the last command was unknown.
// inputs: void
// output: 0 = ok, 1 = remove this player, 2 = shutdown game
int Cmd_Unknown::execute()
{
   _player->message("NOP ");
   _player->message(_command);
   _player->message("\n");
   return 0;
}

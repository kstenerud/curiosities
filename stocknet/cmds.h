#ifndef _CMDS_H
#define _CMDS_H

#include "socklist.h"
#include "textstr.h"
#include "player.h"

extern PlayerList Plist;

// Command abstract base class
class Cmd
{
public:
   Cmd(Player* plr): _player(plr) {}
   virtual ~Cmd() {}
   virtual int  execute()=0;
protected:
   Player* _player;
};

// get a command from a player
Cmd* get_cmd(Player*, Textstring&);

// these ones parse differently depending on player state
Cmd* get_cmd_askname(Player*, Textstring&);
Cmd* get_cmd_askpass(Player*, Textstring&);
Cmd* get_cmd_asknew (Player*, Textstring&);
Cmd* get_cmd_newpass(Player*, Textstring&);
Cmd* get_cmd_playing(Player*, Textstring&);

//==============================================================================

// these "commands" aren't really commands.  They are designed to take special
// information from the user depending on user status.
// eg. get user name if status is askname, get password if status is askpass.

class Cmd_Load_Player: public Cmd	// try to load the player
{
public:
   Cmd_Load_Player(Player* plr, Textstring name): Cmd(plr), _name(name) {}
   ~Cmd_Load_Player() {}
   int execute();
private:
   Textstring _name;
};

class Cmd_Check_Passwd: public Cmd	// match the password
{
public:
   Cmd_Check_Passwd(Player* plr, Textstring pass): Cmd(plr), _pass(pass) {}
   ~Cmd_Check_Passwd() {}
   int execute();
private:
   Textstring _pass;
};

class Cmd_New_YN: public Cmd		// get yes or no answer
{
public:
   Cmd_New_YN(Player* plr, Textstring reply): Cmd(plr), _reply(reply) {}
   ~Cmd_New_YN() {}
   int execute();
private:
   Textstring _reply;
};

class Cmd_New_Pass: public Cmd		// get new password
{
public:
   Cmd_New_Pass(Player* plr, Textstring pass): Cmd(plr), _pass(pass) {}
   ~Cmd_New_Pass() {}
   int execute();
private:
   Textstring _pass;
};

//==============================================================================

// these ones are the real commands:

class Cmd_Bank: public Cmd		// sell all stocks for cash
{
public:
   Cmd_Bank(Player* plr): Cmd(plr) {}
   ~Cmd_Bank() {}
   int execute() {_player->cashout();_player->stat(); return 0;}
};

class Cmd_Shutdown: public Cmd		// kill the whole system
{
public:
   Cmd_Shutdown(Player* plr, Textstring& reason): Cmd(plr), _reason(reason) {}
   ~Cmd_Shutdown() {}
   int execute();
private:
   Textstring _reason;
};

			// it's a knick knack, Pattywhack, give the frog a loan!
class Cmd_Loan: public Cmd
{
public:
   Cmd_Loan(Player* plr): Cmd(plr) {}
   ~Cmd_Loan() {}
   int execute();
};

class Cmd_Fame: public Cmd		// hall of fame -- NOT IMPLEMENTED --
{
public:
   Cmd_Fame(Player* plr): Cmd(plr) {}
   ~Cmd_Fame() {}
   int execute();
};

class Cmd_Who: public Cmd		// who's online
{
public:
   Cmd_Who(Player* plr): Cmd(plr) {}
   ~Cmd_Who() {}
   int execute();
};

class Cmd_Stat: public Cmd		// current player stats
{
public:
   Cmd_Stat(Player* plr): Cmd(plr) {}
   ~Cmd_Stat() {}
   int execute();
};

class Cmd_Quit: public Cmd		// I wanna quit
{
public:
   Cmd_Quit(Player* plr): Cmd(plr) {}
   ~Cmd_Quit() {}
   int execute() {_player->quit(); return 1;}
};

class Cmd_Buy: public Cmd		// buy some stuff
{
public:
   Cmd_Buy(Player* plr, Textstring& arg1, Textstring& arg2, Textstring& arg3)
   : Cmd(plr), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
   ~Cmd_Buy() {}
   int execute();
private:
   Textstring _arg1;			// stock to buy
   Textstring _arg2;			// how many to buy
   Textstring _arg3;			// at what price
};

class Cmd_Sell: public Cmd		// sell some stuff
{
public:
   Cmd_Sell(Player* plr, Textstring& arg1, Textstring& arg2, Textstring& arg3)
   : Cmd(plr), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
   ~Cmd_Sell() {}
   int execute();
private:
   Textstring _arg1;			// stock to sell
   Textstring _arg2;			// how many to sell
   Textstring _arg3;			// at what price
};

class Cmd_Say: public Cmd		// say something to everyone
{
public:
   Cmd_Say(Player* plr, Textstring& msg): Cmd(plr), _msg(msg) {}
   ~Cmd_Say() {}
   int execute();
private:
   Textstring _msg;			// command the user tried to execute
};

class Cmd_Unknown: public Cmd		// what'd you say???
{
public:
   Cmd_Unknown(Player* plr, Textstring& arg1): Cmd(plr), _command(arg1) {}
   ~Cmd_Unknown() {}
   int execute();
private:
   Textstring _command;			// command the user tried to execute
};

#endif

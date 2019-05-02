#ifndef _PLAYER_H
#define _PLAYER_H

#include "socklist.h"
#include "stock.h"
#include "textstr.h"

// enumeration for player states
typedef enum {askname, askpass, asknew, newpass, playing} Status;

// creates a unique 32-bit integer id number.
// I'm using this to allow Player to find Socket, and vice versa.
// My reasoning is this:
// The SockList object maintains a list of sockets with pending input on them.
// When it comes time to parse that input, we need to know the player the socket
// is attached to.
// The problem is that once the parser takes over, we are dealing with a Player
// object.  The player object needs to have a pointer to the Socket object in
// order to send messages to the remote client.
// This, of course, results in a circular dependancy, which C++ doesn't allow.
// So, I've put in this kludge to allow me to use PlayerList to find a Player
// based on a unique id.
int create_id();

class Player
{
public:
   Player(Socket* sock);
   ~Player() {if(_status == playing) save();}
   Socket* get_sock() {return _sock;}
   Textstring get_name() {return _name;}
   int get_id() {return _id;}
   int get_cash() {return _cash;}
   void set_cash(int cash) {_cash = cash;}
   int buy(int stockno, int quantity, int unit_price);
   int sell(int stockno, int quantity, int unit_price);
   void cashout();
   void quit();
   void set_status(Status stat) {_status = stat;}
   Status get_status() {return _status;}
   void message(char* msg) {_sock->writestring(msg);}
   void message(Textstring msg) {_sock->write(msg);}
   void error(Textstring msg) {message("ERR ");message(msg);}
   int receive_dividend(int stockno, int percent);
   int load();
   int save();
   int is_passwd(Textstring& pass) {return pass == _passwd;}
   void set_passwd(Textstring& pass) {_passwd = pass;}
   void set_name(Textstring& name) {_name = name;}
   void stat();
   void stock_crash(int stockno);
   void stock_split(int stockno);
   void who_entry(Player* dest);
   void loan();
   int is_admin() {return _admin;}
   void reset() {for(int i=0;i<6;i++) _stock_quantity[i] = 0; _cash = 500000;}
private:
   Stock*     _stocks[6];		// pointer to active stocks
   int        _stock_quantity[6];	// how many of each we have
   Socket*    _sock;			// socket we are connected to
   Textstring _name;			// name
   Textstring _passwd;			// password
   int        _cash;			// cash
   Status     _status;			// current player state
   int        _id;			// unique id
   int        _loan;			// do we have a loan?
   int        _admin;			// are we an admin?
};

class PlayerList
{
public:
   PlayerList(): _len(1000) {for(int i=0;i<_len;i++) _list[i] = NULL;}
   Player* find(Textstring name);
   Player* find(int id);
   int add(Player* player);
   int remove(Player* player);
   int size();
   void dividend_all(int stockno, int percent);
   void stock_split(int stockno);
   void stock_crash(int stockno);
   void stat_all();
   void msg_all(Textstring message);
   void Who_List(Player* to);
   void fame_list(Player* to);
   void save_all();
   void reset_players();
   void declare_winner();
private:
   // yes it's hardcoded.. there's not enough time to make it elegant.
   Player* _list[1000];			// list of players
   int     _len;			// how big the list is
};

#endif

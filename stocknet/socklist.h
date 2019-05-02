#ifndef _SOCKLIST_H
#define _SOCKLIST_H

// Here's where we actually interact with the sockets.  Socket and Socket_list
// are fairly tightly dependant on one-another (since Socket has the list
// pointers within it).  I did it this way because it results in faster code
// at the expense of easily extensible code.  This may change later.

// regular includes
#include <stdio.h>
#include <string.h>

// networking includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>

#include "textstr.h"
class Player;


// The Socket class handles everything needed to connect and communicate with
// a socket conenction.  I've also put list pointers (_prev and _next) in here
// to speed up list operations.  This may change later.
class Socket
{
public:
           Socket(int maxlen = 1000)
             : _plr_id(0), _socket(0), _server_socket(0), _connected(0)
             , _max_input_len(maxlen), _buffer("", maxlen*4)
             , _current_cmd("", maxlen), _next(NULL), _prev(NULL)
                         {memset(&_sockaddr_in, 0, sizeof(struct sockaddr_in));}
           ~Socket();

   void attach_player(int id) {_plr_id = id;}
   Player* get_player();
   // read len characters from the socket and place them in buf.
   int     read(char* buf, int len)
                                    {return ::read(_socket, buf, len);}

   // write a null-terminated string to the socket.
   int     writestring(char* buf) {return ::write(_socket, buf, ::strlen(buf));}

   // write a Textstring to the socket.
   int     write(Textstring& txt)
                  {return ::write(_socket, txt.get_string(), txt.get_length());}

   // write len bytes of the array buf to the socket.
   int     write(char* buf, int len) {return ::write(_socket, buf, len);}

   // write one byte to the socket.
   int     writebyte(char ch)       {return ::write(_socket, &ch, 1);}

   // get the internet dotted quad address of the caller.
   char*   get_addr()               {return ::inet_ntoa(_sockaddr_in.sin_addr);}

   // get the port the caller is connected on.
   u_short get_port()               {return _sockaddr_in.sin_port;}

   // get the file descriptor the socket is on.
   int     get_sockfd()             {return _socket;}

   // are we connected?
   int     connected()              {return _connected != 0;}

   // connect a new socket from the server socket sock.
   // the server socket must be set up using socket(), bind(), and listen(),
   // which are handled by Socket_list.
   int     connect(int sock);

   // disconnect a socket.
   int     disconnect();

   // purge the input buffer of a socket.
   void    purge_buffer()           {_buffer = "";}

   // get input from the socket and store it in the input buffer.  If there's
   // a newline in the buffer and the current commadn buffer is empty,
   // then move the contents up to the newline into the current command buffer.
   int     add_input();

   // get the current command to be interpreted.
   Textstring get_current_cmd() {return _current_cmd;}

   // clear the current command buffer.
   void    clear_current_cmd()      {_current_cmd = "";}

   // is there a command to be interpreted?
   int     data_ready()             {return !_current_cmd.is_empty();}

   // add current socket to a socket list.
   Socket* add_to_list(Socket* head);

   // remove current socket from the list and destroy this socket.
   Socket* remove_from_list();

   // get the next socket in the list.
   Socket* next()                   {if(this == NULL) return NULL;return _next;}

   // get the previous socket in the list.
   Socket* prev()                   {if(this == NULL) return NULL;return _prev;}

private:
          int         _socket;		// the socket fd we are connected to
   struct sockaddr_in _sockaddr_in;	// for the hostname and port
          int         _server_socket;	// sockfd of the server socket
          int         _connected;	// flag
          Textstring  _buffer;		// input buffer
          Textstring  _current_cmd;	// current command buffer
          int         _max_input_len;	// determines the buffer size
          int         _plr_id;		// player ID
          Socket*     _prev;		// prev in socket list
          Socket*     _next;		// next in socket list
};


// The socket list works in conjunction with Socket objects to implement a list
// of sockets.  The socket list also handles initialization of the server
// socket.
class Socket_list
{
public:
   Socket_list(int port, int maxlen = FD_SETSIZE);
   ~Socket_list();

   // add a socket to the list.
   int     add(Socket* sock);

   // remove a socket from the list.
   Socket* remove(Socket* sock);

   // get the first socket in the list.
   Socket* first();

   // get the next socket in the list.
   Socket* next();

   // check if the list is empty.
   int     empty()                             {return _length == 0;}

   // check if the list is full.
   int     full()                              {return _length == _max_length;}

   // check for new connections on the server socket and add them to the list.
   Socket* add_new_connection();

   // wait for activity on the sockets.
   int     wait_for_input(int wait);

   // get input from the sockets and buffer them.
   void    gather_input();

   // get the first socket with pending commands.
   Socket* first_input()             {_current_in = _start;return next_input();}

   // get the next socket with pending commands.
   Socket* next_input();

private:
   Socket* _start;		// start of the socket list
   Socket* _end;		// end of the socket list
   Socket* _current;		// placeholder for first() and next()
   Socket* _current_in;		// placeholder for first_input and next_input
   int     _length;		// length of the list
   int     _max_length;		// maximum sockets in list
   int     _server_fd;		// server file descriptor
   int     _max_fd;		// used by the select() system call.
   fd_set  _in_set;		// used by the select() system call.
   fd_set  _out_set;		// used by the select() system call.
   fd_set  _exc_set;		// used by the select() system call.
};

#endif

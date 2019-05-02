#ifndef _SOCKET_H
#define _SOCKET_H

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <X11/Intrinsic.h>
#include "baerg.h"

class Socket;
class SockNode;
class SockList;
class SockIter;
class SockManager;

void Accept_callback(XtPointer clientData, int* fd, XtInputId* id);
void Input_callback(XtPointer clientData, int* fd, XtInputId* id);
void Error_callback(char* error, char* desc);


class Socket
{
public:
                  Socket(XtAppContext app, void(* input_callback)(XtPointer clientData, int* fd, XtInputId* id), void(* error_callback)(char* error, char* desc))
                  : _connected(FALSE), _app(app), _input_callback(input_callback)
                  , _input_id(0), _id(0), _fd(0), _server(FALSE), _accept_mode(FALSE) {}
                  ~Socket() {disconnect();}
   int            connect(char* address, unsigned short port);
   int            accept(int server_fd);
   int            accept(Socket& server) {return accept(server.get_fd());}
   int            server(unsigned short port);
   int            disconnect();
   int            read(char* buff, int len);
   int            write(char* buff, int len);
   int            write(char* str) {return write(str, strlen(str)+1);}
   int            get_fd() {return _connected ? _fd : -1;}
   int            is_connected() {return _connected;}
   XtInputId      get_input_id() {return _input_id;}
   unsigned short get_port() {return ntohs(_saddr.sin_port);}
   char*          get_address() {return inet_ntoa(_saddr.sin_addr);}
   unsigned long  get_ip() {return ntohl(_saddr.sin_addr.s_addr);}
   void           set_id(unsigned char id) {_id = id;}
   unsigned char  get_id() {return _id;}
   void           add(Socket* sock);
   void           remove(Socket* sock);
   void           set_server() {_server = TRUE;}
   void           unset_server() {_server = FALSE;}
   int            is_server() {return _server;}
   int            accept_mode() {return _accept_mode;}
private:
   int                _fd;		// file descriptor
   int                _connected;	// flag if connected
   int                _server;		// flag if this is a server socket
   struct sockaddr_in _saddr;		// holds socket info
   XtAppContext       _app;		// app context of X window
   XtInputId          _input_id;	// input ID given by Xt
   unsigned char      _id;		// id given by SockManager
   int                _accept_mode;	// flag if they contacted us
					// callback if input occurs
   void(*             _input_callback)(XtPointer clientData, int* fd, XtInputId* id);
					// callback if an error occurs
   void(*             _error_callback)(char* error, char* desc);
};


class SockNode
{
   friend class SockList;
   friend class SockIter;
private:
   SockNode(Socket* sock, SockNode* prev, SockNode* next)
   : _sock(sock), _prev(prev), _next(next) {}
   Socket*   _sock;			// socket
   SockNode* _prev;			// prev node
   SockNode* _next;			// next node
private:
};

class SockList
{
   friend class SockIter;
public:
        SockList(): _head(NULL), _size(0) {}
        ~SockList();
   void add(Socket* sock);
   void remove(Socket* sock);
   int  get_size() {return _size;}
private:
   SockNode* _head;			// head of list
   int       _size;			// size of list
};

class SockIter
{
public:
           SockIter(SockList* list): _list(list), _curr(list->_head) {}
   void    reset() {_curr = _list->_head;}
   Socket* next();
private:
   SockList* _list;			// list
   SockNode* _curr;			// current point in list
};


class SockManager
{
friend void Accept_callback(XtPointer clientData, int* fd, XtInputId* id);
friend void Input_callback(XtPointer clientData, int* fd, XtInputId* id);
friend void Error_callback(char* error, char* desc);
public:
                 SockManager(unsigned short port, XtAppContext app, void(* draw_callback)(unsigned char id, unsigned char operation, char* data, int len), void(* error_callback)(char* error, char* desc), int max_connect);
                 ~SockManager() {disconnect(); _instance = NULL;}
   unsigned char get_id() {return _my_id;}
   Socket*       wait_input_or_connect(int time);
   Socket*       first() {_iter.reset(); return next();}
   Socket*       next() {return _iter.next();}
   int           get_max_connect() {return SockManager::_max_connect;}
   void          set_max_connect(int val) {SockManager::_max_connect = val;}
   int           num_connected() {return _list.get_size();}
   unsigned char find_free_id();
   Socket*       get_socket(unsigned char id);
   Socket*       get_socket(int fd);
   Socket*       get_socket(XtInputId id);
   void          broadcast_operation(unsigned char operation, char* data, int len);
   int           send_packet(Socket* sock, unsigned char operation, char* data, int len);
   void          add(Socket* sock);
   void          remove(Socket* sock, int force = FALSE);
   void          disconnect();
   void          new_server();
   void          connect(char* address, unsigned short port = B_PORT);
   void          connect(unsigned long ip, unsigned short port);
   void          handle_connect(Socket* sock, unsigned char* packet, int len);
   int           send_req_server_packet(Socket* sock);
   int           send_i_am_server_packet(Socket* sock, unsigned char server_id, unsigned char your_id);
   int           send_connect_req_packet(Socket* sock, unsigned long server_ip, unsigned char id);
   int           send_connect_ack_packet(Socket* sock, unsigned char id);
   int           send_connect_nak_packet(Socket* sock, unsigned long server_ip);
   static SockManager* Instance();
   static int Init(unsigned short port, XtAppContext app, void(* draw_callback)(unsigned char id, unsigned char operation, char* data, int len),  void(* error_callback)(char* error, char* desc), int max_connect);
private:
   void          _accept_callback(XtPointer clientData, int* fd, XtInputId* id);
   void          _input_callback(XtPointer clientData, int* fd, XtInputId* id);
   void(*        _error_callback)(char* error, char* desc);
   static SockManager*   _instance;	// singleton instance of this SockManager
   unsigned short _port;		// port number we will use
   XtAppContext   _app;			// app context of X Window
   int            _max_connect;		// max connections allowed
   Socket         _server;		// server socket
   SockList       _list;		// list of connected sockets
   SockIter       _iter;		// iterator for that list
					// callback for draw operations
   void(*  _draw_callback)(unsigned char id, unsigned char operation, char* data, int len);
   unsigned char  _my_id;		// my id
   int            _multi;		// flag for multi or single
   int            _is_server;		// flag if we are the server
   unsigned long  _server_ip;		// server's IP address
   unsigned char  _server_id;		// server's id
};

#endif

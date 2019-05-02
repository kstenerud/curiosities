//------------------------------------------------------------------------------
//FILE:	        socket.c
//DATE:		May 9, 97
//REVISION:     
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//NOTES:        Everything to do with the socket layer.
//------------------------------------------------------------------------------

#include "socket.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "baerg.h"



//------------------------------------------------------------------------------
//FUNCTION:	Socket::server
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::server(unsigned short port)
//		-port: the port number to listen on
//RETURNS:	TRUE/FALSE
//DEPENDENCIES: none
//PRECONDITION: socket can't be connected.
//NOTES:	Sets a socket to "server" mode, which listens on the selected
//		port.
//		when using server(), get_address() and get_port() will return
//		the LOCAL address and port since it would be meaningless to
//		return the "remote" ones.
//------------------------------------------------------------------------------
int Socket::server(unsigned short port)
{
   static int on = 1;		// used by setsockopt() to turn something "on"
   static int len;		// used by getsockname()
				// these variables have to be static for the
				// above functions to work properly within
				// an object... I'm not too sure why.

   // make sure we're not already connected
   if(_connected)
      return FALSE;

   ::memset(&_saddr, 0, sizeof(_saddr));
   _saddr.sin_family = AF_INET;			// address family
   _saddr.sin_port   = htons(port);		// host port number

   // open socket
   if( (_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0 )
   {
      ::perror("Socket::server: socket");
      return FALSE;
   }

   // options
   if(::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on)) < 0)
   {
       ::perror( "Socket::server: setsockopt" );
       disconnect();
       return FALSE;
   }

   // bind host and port to socket
   if( ::bind(_fd,(struct sockaddr*)&_saddr,sizeof(_saddr)) < 0 )
   {
      ::perror("Socket::server: bind");
      disconnect();
      return FALSE;
   }

   // allow up to 3 queued connects
   ::listen(_fd, 3);

   // save socket information
   if( ::getsockname(_fd, (struct sockaddr*)&_saddr, &len) < 0 )
   {
      ::perror("Socket::server: getsockname");
      disconnect();
      return FALSE;
   }

   // we are now connected
   _connected = TRUE;

   if(_input_callback != NULL)
      _input_id = XtAppAddInput(_app, _fd, (XtPointer) XtInputReadMask, _input_callback, NULL);

   _accept_mode = FALSE;

   return TRUE;
}


//------------------------------------------------------------------------------
//FUNCTION:	Socket::accept
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::accept(int server_fd)
//		-server_fd: the file descriptor to accept a connection from
//RETURNS:	TRUE/FALSE
//DEPENDENCIES: none
//PRECONDITION: socket can't be connected
//NOTES:	Opens a socket, taking the pending connection from a server
//              socket.
//------------------------------------------------------------------------------
int Socket::accept(int server_fd)
{
   static int size = sizeof(_saddr);		// used by accept()
   static int len;				// used by getpeername()

   if(_connected)
      return FALSE;

   // accept the connection
   if( (_fd = ::accept(server_fd, (struct sockaddr*) &_saddr, &size)) < 0 )
   {
      perror("Socket::accept: accept");
      return FALSE;
   }

   // get some info
   if( ::getpeername(_fd, (struct sockaddr*)&_saddr, &len) < 0 )
   {
      ::perror("Socket::accept: getpeername");
      disconnect();
      return FALSE;
   }

   _connected = TRUE;

   if(_input_callback != NULL)
      _input_id = XtAppAddInput(_app, _fd, (XtPointer) XtInputReadMask, _input_callback, NULL);

   _accept_mode = TRUE;

   return TRUE;
}


//------------------------------------------------------------------------------
//FUNCTION:	Socket::connect
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::connect(char* address, unsigned short port)
//		-address: the address of the remote host
//		-port: the port number to connect to
//RETURNS:	TRUE/FALSE
//DEPENDENCIES: none
//PRECONDITION: socket can't be conencted.
//NOTES:	The address can be either in hostname format or dotted quad
//		format.
//------------------------------------------------------------------------------
int Socket::connect(char* address, unsigned short port)
{
   struct hostent* host_info;		// returned by gethostbyname()
   static int len;			// used by getpeername()

   if(_connected)
      return FALSE;
   // prepare socket info
   ::memset(&_saddr, 0, sizeof(_saddr));
   _saddr.sin_family = AF_INET;
   _saddr.sin_port = htons(port);

   // try to decode dotted quad notation
   if(!::inet_aton(address, &_saddr.sin_addr))
   {
      // failing that, look up the name
      if( (host_info = ::gethostbyname(address)) == NULL)
      {
         ::perror("Socket::connect: gethostbyname");
         return FALSE;
      }
      ::memcpy(&_saddr.sin_addr, host_info->h_addr, host_info->h_length);
   }

   // open the socket
   if( (_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      ::perror("Socket::connect: socket");
      return FALSE;
   }

   // connect to the remote site
   if(::connect(_fd, (struct sockaddr*) &_saddr, sizeof(_saddr)) < 0)
   {
      ::perror("Socket::connect: connect");
      disconnect();
      return FALSE;
   }

   // get some info
   if( ::getpeername(_fd, (struct sockaddr*)&_saddr, &len) < 0 )
   {
      ::perror("Socket::connect: getpeername");
      disconnect();
      return FALSE;
   }

   _connected = TRUE;

   if(_input_callback != NULL)
      _input_id = XtAppAddInput(_app, _fd, (XtPointer) XtInputReadMask, _input_callback, NULL);

   _accept_mode = FALSE;

   return TRUE;
}


//------------------------------------------------------------------------------
//FUNCTION:	Socket::disconnect
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::disconnect()
//RETURNS:	TRUE/FALSE
//DEPENDENCIES: none
//PRECONDITION: socket must be connected.
//NOTES:	Disconnects the socket and removes the XInput callback.
//              returns false if the socket is not connected.
//------------------------------------------------------------------------------
int Socket::disconnect()
{
   // don't close an already closed socket
   if(!_connected)
      return FALSE;

   XtRemoveInput(_input_id);

   ::close(_fd);
   _connected = FALSE;

   return TRUE;
}


//------------------------------------------------------------------------------
//FUNCTION:	Socket::read
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::read(char* buff, int len)
//		-buff: the buffer to read into
//		-len: number of bytes to read
//RETURNS:	int: number of bytes actually read (-1 if it failed).
//DEPENDENCIES: none
//PRECONDITION: socket must be connected.
//NOTES:	reads len bytes from this socket into buff.
//------------------------------------------------------------------------------
int Socket::read(char* buff, int len)
{
   int length;				// number of bytes read

   // make sure we CAN read
   if(!_connected || buff == NULL || len <= 0)
      return 0;

   if((length=::read(_fd, buff, len)) <= 0)
      disconnect();
   return length;
}


//------------------------------------------------------------------------------
//FUNCTION:	Socket::write
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::write(char* buff, int len)
//		-buff: the buffer to write from
//		-len: number of bytes to write
//RETURNS:	int: number of bytes actually written (-1 if it failed).
//DEPENDENCIES: none
//PRECONDITION: socket must be connected.
//NOTES:	writes len bytes from buff to this socket.
//------------------------------------------------------------------------------
int Socket::write(char* buff, int len)
{
   int length;				// number of bytes written

   // make sure we CAN write
   if(!_connected || buff == NULL || len <= 0)
      return 0;

   if((length=::write(_fd, buff, len)) <= 0)
      disconnect();
   return length;
}


//******************************************************************************
//******************************************************************************

//------------------------------------------------------------------------------
//FUNCTION:	Accept_callback
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void Accept_callback(XtPointer clientData, int* fd, XtInputId* id)
//		-clientData: client specific data (not used)
//		-fd: the file descriptor that has pending input
//		-id: the id of this input hook
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: This function must be registered with X as a callback procedure.
//NOTES:	Handles accepting of connections on a server socket.
//		All this function does is pass the information on to the
//              SockManager's accept callback.
//------------------------------------------------------------------------------
void Accept_callback(XtPointer clientData, int* fd, XtInputId* id)
{
   // Just pass it on to the sockmanager
   SockManager* smgr = SockManager::Instance();
   if(smgr != NULL)
      smgr->_accept_callback(clientData, fd, id);
}


//------------------------------------------------------------------------------
//FUNCTION:	Input_callback
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void Input_callback(XtPointer clientData, int* fd, XtInputId* id)
//		-clientData: client specific data (not used)
//		-fd: the file descriptor that has pending input
//		-id: the id of this input hook
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: This function must be registered with X as a callback procedure.
//NOTES:	Handles input on sockets.
//		All this function does is pass the information on to the
//              SockManager's input callback.
//------------------------------------------------------------------------------
void Input_callback(XtPointer clientData, int* fd, XtInputId* id)
{
   // Just pass it on to the sockmanager
   SockManager* smgr = SockManager::Instance();

   if(smgr != NULL)
      smgr->_input_callback(clientData, fd, id);
}


//------------------------------------------------------------------------------
//FUNCTION:	Error_callback
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void Error_callback(char* error, char* desc)
//		-error: the error and where it happened
//		-desc: more details about the error
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	This function is meant to act as a hook to a higher level function
//		to report errors to the user.
//		All this function does is pass the information on to the
//              SockManager's error callback.
//------------------------------------------------------------------------------
void Error_callback(char* error, char* desc)
{
   // Just pass it on to the sockmanager
   SockManager* smgr = SockManager::Instance();
   if(smgr != NULL)
      smgr->_error_callback(error, desc);
}


SockManager* SockManager::_instance = NULL;	// holds a singleton instance of
						// a SockManager object


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::Instance
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	SockManager* SockManager::Instance()
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: You must run Socket::Init() first.
//NOTES:	Returns the singleton instance of SockManager initialized by
//		SockManager::Init().
//------------------------------------------------------------------------------
SockManager* SockManager::Instance()
{
   return _instance;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::Init
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::Init(unsigned short port, XtAppContext app,
//		   void(* draw_callback)(unsigned char id, unsigned char operation,
//				         char* data, int len),
//		   void(* error_callback)(char* error, char* desc), int max_connect)
//		-port: the port number to listen on.
//		-app: the X application context (for Input callbacks).
//		-draw_callback: procedure to invoke when the SockManager recieves
//				a draw packet.
//		-error_callback: procedure to invoke to alert the user to an error.
//		-max_connect: maximum number of simultaneous connections.
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Initializes a singleton of a SockManager.
//------------------------------------------------------------------------------
int SockManager::Init(unsigned short port, XtAppContext app, void(* draw_callback)(unsigned char id, unsigned char operation, char* data, int len),  void(* error_callback)(char* error, char* desc), int max_connect)
{
   if(_instance != NULL);
   {
      _instance = new SockManager(port, app, draw_callback, error_callback,
                                  max_connect);
      return TRUE;
   }
   return FALSE;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::SockManager (constructor)
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::SockManager(unsigned short port, XtAppContext app,
//		   void(* draw_callback)(unsigned char id, unsigned char operation,
//				         char* data, int len),
//		   void(* error_callback)(char* error, char* desc), int max_connect)
//		-port: the port number to listen on.
//		-app: the X application context (for Input callbacks).
//		-draw_callback: procedure to invoke when the SockManager recieves
//				a draw packet.
//		-error_callback: procedure to invoke to alert the user to an error.
//		-max_connect: maximum number of simultaneous connections.
//RETURNS:	none
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	-
//------------------------------------------------------------------------------
SockManager::SockManager(unsigned short port, XtAppContext app, void(* draw_callback)(unsigned char id, unsigned char operation, char* data, int len), void(* error_callback)(char* error, char* desc), int max_connect)
: _iter(&_list), _max_connect(max_connect), _app(app), _multi(FALSE)
, _is_server(FALSE), _server(app, ::Accept_callback, ::Error_callback)
, _server_ip(0), _server_id(0), _error_callback(error_callback)
, _draw_callback(draw_callback)
{
   // set up the server socket
   _server.server(port);

   // failure at this point can't be recovered from, so we exit
   if(!_server.is_connected())
   {
      ::fprintf(stderr, "Could not initialize server socket.\n");
      exit(1);
   }
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::connect
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::connect(char* address, unsigned short port)
//		-address: the address of the remote host
//		-port: the port number to connect to
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Makes a new socket, connects it to the remote site, and adds it to
//		the SockManager's socket list.
//		The address can be either in hostname format or dotted quad
//		format.
//------------------------------------------------------------------------------
void SockManager::connect(char* address, unsigned short port)
{
   Socket* sock = new Socket(_app, ::Input_callback, ::Error_callback);

   if(num_connected() == 0)
   {
      _is_server = FALSE;
      _server_ip = 0;
      _multi = TRUE;
      _my_id = 0;
   }

   if(!sock->connect(address, port))
   {
      Error_callback("SockManager::connect", "Couldn't connect to host");
      return;
   }

   add(sock);

   if(_server_ip == 0)		// first time connect
      send_req_server_packet(sock);
   else
      send_connect_req_packet(sock, _server_ip, _my_id);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::connect
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockManager::connect(unsigned long ip, unsigned short port)
//		-ip: the address in the form of a long int
//		-port: the port number to connect to
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Makes a new socket, connects it to the remote site, and adds it to
//		the SockManager's socket list.
//		Same as the previous connect, but it lets you use the 32-bit int
//		form of the address instead.
//------------------------------------------------------------------------------
void SockManager::connect(unsigned long ip, unsigned short port)
{
   struct in_addr inaddr;

   inaddr.s_addr = ip;
   connect(inet_ntoa(inaddr), port);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::disconnect
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int Socket::disconnect()
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Disconnect all sockets on the list.
//------------------------------------------------------------------------------
void SockManager::disconnect()
{
   SockIter iter(&_list);
   Socket* sock;

   for(sock = iter.next();sock != NULL;sock = iter.next())
      remove(sock);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::_accept_callback
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void _accept_callback(XtPointer clientData, int* fd, XtInputId* id)
//		-clientData: client specific data (not used)
//		-fd: the file descriptor that has pending input
//		-id: the id of this input hook
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Handles accepting of connections on a server socket.
//------------------------------------------------------------------------------
void SockManager::_accept_callback(XtPointer clientData, int* fd, XtInputId* id)
{
   Socket* sock = new Socket(_app, ::Input_callback, ::Error_callback);

   sock->accept(_server);

   if(!sock->is_connected())
   {
      delete sock;
      Error_callback("SockManager::_accept_callback", "Accept failed");
      return;
   }
   if(num_connected() == 0)	// not connected anywhere
   {
      _is_server = TRUE;	// so we are the server
      _my_id = 1;
      _multi = TRUE;		// default to multi mode
   }
   add(sock);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::_input_callback
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void _input_callback(XtPointer clientData, int* fd, XtInputId* id)
//		-clientData: client specific data (not used)
//		-fd: the file descriptor that has pending input
//		-id: the id of this input hook
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Handles input on sockets.
//------------------------------------------------------------------------------
void SockManager::_input_callback(XtPointer clientData, int* fd, XtInputId* id)
{
   Socket* sock = get_socket(*fd);	// get the socket object
   unsigned char packet[513];		// in case someone forgets to null terminate
   int len = 0;				// counts bytes read
   int data_len;			// length of bytes that should be read
   unsigned char operation;		// operation code
   int multi;				// flag if multi-connect or note
   char str[100];			// temp buffer for error callback string
 
   if(sock == NULL)
      return;

   // read the baerg header information
   if( (len=sock->read((char*)packet, 2)) < 2)
   {
      sprintf(str, "read failed or user %d closed connection\n", sock->get_id());
      remove(sock, TRUE);
      Error_callback("SockManager::_input_callback", str);
      return;
   }

   data_len = (*packet)*2;
   operation = *(packet+1) & 0x7f;
   multi = ( (*(packet+1) & 0x80) ? TRUE : FALSE);
   memset(packet, 0, 513);

   // now read the rest of the packet, if there is anything else
   if(data_len > 0)
   {
      if( (len=sock->read((char*)packet, data_len)) < data_len )
      {
         if(len < 0)
         {
            sprintf(str, "read failed on user %d\n", sock->get_id());
            remove(sock, TRUE);
            Error_callback("SockManager::_input_callback", str);
         }
         return;
      }
   }

   if(!multi)				// client wanted single
   {
      if(num_connected() == 1)		// they're the only one connected
      {
         _multi = FALSE;		// go to single mode
         _my_id = 1;			// we're id 1
         sock->set_id(2);		// they're id 2
         _server_id = _my_id;		// server is us
         _server_ip = _server.get_ip();
         _is_server = TRUE;
      }
      else				// but others are connected
      {
         remove(sock);
         return;
      }
   }


   switch(operation)
   {
   case B_OP_INIT_MULTI:
      if(multi)				// only multi mode users can send this
         handle_connect(sock, packet, data_len);
      break;
   case B_OP_INITIALIZE:
      break;
   case B_OP_DISCONNECT:
      remove(sock, TRUE);
      break;
   case B_OP_RESTORE_REQ:
   case B_OP_RESTORE_ACK:
   case B_OP_RESTORE_NAK:
   case B_OP_RESTORE_DATA:
   case B_OP_RESTORE_END:
      send_packet(sock, B_OP_RESTORE_NAK, NULL, 0);	// we don't do restores
      break;
   default:
      _draw_callback(sock->get_id(), operation, (char*)packet, data_len);
   }
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::handle_connect
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockManager::handle_connect(Socket* sock,
//		                                 unsigned char* packet, int len)
//		-sock: the socket negotiating a multi-connect
//		-packet: the packet sent on this socket
//		-len: length of the packet
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	This handles the handshaking phase of the multi-client
//		connection
//------------------------------------------------------------------------------
void SockManager::handle_connect(Socket* sock, unsigned char* packet, int len)
{
   unsigned char* packet_data = packet;		// extra pointer to pakcet data
   unsigned long ip = 0;			// holds an IP address
   unsigned char id = 0;			// holds an id
   unsigned char message;			// holds the message type
   unsigned char num;				// number of other sockets
   int i;					// counter

   message = *packet_data++;

   _multi = TRUE;

   if(sock->accept_mode())			// they connected to us
   {
      switch(message)
      {
      case B_MULTI_REQ_SERVER:			// looking for the server
         if(num_connected() == 1)		// looks like we're the server
            _is_server = TRUE;
         if(_is_server)				// we're the server
         {
            if(_my_id == 0)			// first time connect?
               _my_id = 1;			// we're id 1
            _server_id = _my_id;		// server is us
            _server_ip = _server.get_ip();
            sock->set_id(find_free_id());	// find an id for them
            if(sock->get_id() > 0)
               send_i_am_server_packet(sock, _my_id, sock->get_id());
            else				// we're full
            {
               send_connect_nak_packet(sock, 0);
               remove(sock);
            }
         }
         else					// we're not the server
         {
            send_connect_nak_packet(sock, _server_ip);
            remove(sock);
         }
         break;

      case B_MULTI_CONNECT_REQ:
         memcpy((char*)&ip, packet_data, 4);
         id = *(packet_data+4);

         // make sure they know who the server is, and their id is valid
         if(ip == _server_ip && get_socket(id) == NULL && id > 0 && id <= _max_connect)
         {
            sock->set_id(id);
            send_connect_ack_packet(sock, _my_id);
         }
         else
         {
            send_connect_nak_packet(sock, _server_ip);
            remove(sock);
         }

         break;
      default:					// received a bad message
         remove(sock);				// just remove them
      }
   }
   else						// we connected to them
   {
      switch(message)
      {
      case B_MULTI_I_AM_SERVER:
         _server_id = *packet_data++;
         _my_id = *packet_data++;
         num = *packet_data++;
         sock->set_id(_server_id);
         _server_ip = sock->get_ip();

         for(i=0;i<num;i++)			// connect to all other hosts
         {
            memcpy((char*)&ip, packet_data, 4);
            packet_data += 4;
            connect(ip, B_PORT);
         }
         break;
      case B_MULTI_CONNECT_ACK:
         id = *packet_data;
         if((id > 0 && id <= _max_connect) || get_socket(id) == NULL)
            sock->set_id(id);
         else
            remove(sock);
         break;
      case B_MULTI_CONNECT_NAK:
         memcpy((char*)&ip, packet_data, 4);
         disconnect();			// disconnect from all sockets
         if(ip != 0)			// if it wasn't a fatal error, connect to server
            connect(ip, B_PORT);
         break;
      default:				// received a bad message
         remove(sock);			// just remove them
      }
   }
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::send_packet
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::send_packet(Socket* sock, unsigned char operation,
//		                             char* data, int len)
//		-sock: the socket to send the packet to.
//		-operation: the operation of this packet.
//		-data: the data of this packet.
//		-len: length of the data.
//RETURNS:	int: the number of bytes written to the socket or -1 for error.
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	Encodes and sends a packet over the desired socket.
//------------------------------------------------------------------------------
int SockManager::send_packet(Socket* sock, unsigned char operation, char* data, int len)
{
   unsigned char packet[514];			// packet data
   int pkt_len = len + (len%2 == 0 ? 0 : 1);	// length of data to send
   int retval = 0;				// holds a return value
   char str[100];				// temp string for error callback

   memset(packet, 0, 514);

   // make the header
   packet[0] = (unsigned char) (pkt_len/2);
   packet[1] = (_multi ? B_TYPE_MULTI : B_TYPE_SINGLE) | operation;

   // fill out the data, if any
   if(len > 0)
      memcpy((packet+2), data, len);

   // now send it
   retval = sock->write((char*)packet, pkt_len+2);
   if(!retval)
   {
      sprintf(str, "write failed on user %d", sock->get_id());
      remove(sock, TRUE);
      Error_callback("SockManager::send_packet", str);
   }
   return retval;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::send_req_server_packet
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::send_req_server_packet(Socket* sock)
//		-sock: the socket to send the packet to.
//RETURNS:	int: the number of bytes written to the socket or -1 for error.
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	Sends a "request server" packet over the desired socket.
//		The request server packet asks the remote host who is the
//		server.
//		This is the FIRST packet to be sent when a client connects to
//		its first host and needs to know who is the server.
//------------------------------------------------------------------------------
int SockManager::send_req_server_packet(Socket* sock)
{
   unsigned char packet[2] = {B_MULTI_REQ_SERVER, 0};

   return send_packet(sock, B_OP_INIT_MULTI, (char*)packet, 2);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::send_i_am_server_packet
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::send_i_am_server_packet(Socket* sock, unsigned char server_id,
//		                                         unsigned char your_id)
//		-sock: the socket to send the packet to.
//		-server_id: the ID of the server.
//		-your_id: the ID you are assigning to the client.
//RETURNS:	int: the number of bytes written to the socket or -1 for error.
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	Sends a "i am server" packet over the desired socket.
//		This packet tells the client that we are the server, gives them
//		our id, and assigns an id to them.
//------------------------------------------------------------------------------
int SockManager::send_i_am_server_packet(Socket* sock, unsigned char server_id, unsigned char your_id)
{
   SockIter iter(&_list);			// iterator to find sockets
   Socket* tsock;				// temp socket pointer
   unsigned char packet[512];			// packet to send
   unsigned char* packet_data = packet+4;	// pointer to inside packet
   unsigned long  ip;				// ip address

   memset(packet, 0, 512);

   packet[0] = B_MULTI_I_AM_SERVER;
   packet[1] = server_id;
   packet[2] = your_id;
   packet[3] = (unsigned char) _list.get_size();

   // get a list of all connected sockets
   for(tsock = iter.next();tsock != NULL;tsock = iter.next())
   {
      ip = htonl(tsock->get_ip());
      if(sock->get_ip() == ip)			// don't add their address
         packet[3] = _list.get_size() - 1;
      else
      {
         memcpy((packet_data), (unsigned char*)&ip, 4);
         packet_data += 4;
      }
   }
   return send_packet(sock, B_OP_INIT_MULTI, (char*)packet, packet_data-packet);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::send_connect_req_packet
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::send_connect_req_packet(Socket* sock, unsigned long server_ip,
//		                                         unsigned char id)
//		-sock: the socket to send the packet to.
//		-server_ip: The IP address of who we think the server is.
//		-id: The ID the server assigned to us.
//RETURNS:	int: the number of bytes written to the socket or -1 for error.
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	Sends a "connect request" packet over the desired socket.
//		This tells the other client we've connected to the server, and
//		have been given an ID.  Now we are connecting to all other
//		clients	connected to the server.
//------------------------------------------------------------------------------
int SockManager::send_connect_req_packet(Socket* sock, unsigned long server_ip, unsigned char id)
{
   unsigned char packet[6];

   packet[0] = B_MULTI_CONNECT_REQ;
   memcpy((packet+1), (unsigned char*)&server_ip, 4);
   packet[5] = id;
   return send_packet(sock, B_OP_INIT_MULTI, (char*)packet, 6);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::send_connect_ack_packet
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::send_connect_ack_packet(Socket* sock, unsigned char id)
//		-sock: the socket to send the packet to.
//		-id: The ID the server assigned to us.
//RETURNS:	int: the number of bytes written to the socket or -1 for error.
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	Sends a "connect acknowledge" packet over the desired socket.
//		This is sent in response to a connect request packet.  It tells
//		the other client that they have the right info about the server,
//		and we've added them to our socket list.  We also tell them our
//		ID.
//------------------------------------------------------------------------------
int SockManager::send_connect_ack_packet(Socket* sock, unsigned char id)
{
   unsigned char packet[2];

   packet[0] = B_MULTI_CONNECT_ACK;
   packet[1] = id;

   return send_packet(sock, B_OP_INIT_MULTI, (char*)packet, 2);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::send_connect_nak_packet
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	int SockManager::send_connect_nak_packet(Socket* sock, unsigned long server_ip)
//		-sock: the socket to send the packet to.
//		-ip: The IP of the server for a recoverable error, or 0 for
//		     a non-recoverable one.
//RETURNS:	int: the number of bytes written to the socket or -1 for error.
//DEPENDENCIES: none
//PRECONDITION: Socket must be connected.
//NOTES:	Sends a "connect negative acknowledge" packet over the desired
//		socket.
//		This is sent in response to a connect request packet or a
//		request server packet.
//		It would normally contain the IP address of the server if sent
//		in response to a request server packet, which would inform the
//		client of who the server is.
//		If the IP is 0, this is a non-recoverable error, and the client
//		should give up at this point.  One such condition would be if
//		the server is full.
//------------------------------------------------------------------------------
int SockManager::send_connect_nak_packet(Socket* sock, unsigned long server_ip)
{
   unsigned char packet[6];

   packet[0] = B_MULTI_CONNECT_NAK;
   memcpy((packet+1), (unsigned char*)&server_ip, 4);
   packet[5] = 0;

   return send_packet(sock, B_OP_INIT_MULTI, (char*)packet, 6);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::broadcast operation
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockManager::broadcast_operation(unsigned char operation,
//		                                      char* data, int len)
//		-operation: The operation of the packet to broadcast.
//		-data: the data to put in the packet.
//		-len: the length of that data.
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	This function broadcasts a packet to all connected sockets.
//		It is normally used to broadcast draw operations to the other
//		clients.
//------------------------------------------------------------------------------
void SockManager::broadcast_operation(unsigned char operation, char* data, int len)
{
   SockIter iter(&_list);			// to step through socket list
   Socket* sock;				// temp socket pointer

   iter.reset();
   for(sock = iter.next();sock != NULL;sock = iter.next())
      send_packet(sock, operation, data, len);
} 


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::find_free_id
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	unsigned char SockManager::find_free_id()
//RETURNS:	unsigned char: the ID it found or 0.
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Looks for a free ID number among the connected sockets.
//		If it doesn't find one, it returns 0 (which is not a valid ID).
//------------------------------------------------------------------------------
unsigned char SockManager::find_free_id()
{
   unsigned char i;
   for(i=1;i<=_max_connect&&i<0xff;i++)
      if(i != _my_id)
         if(get_socket(i) == NULL)
            return i;
   return 0;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::get_socket
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	Socket* SockManager::get_socket(int fd)
//		-fd: the file descriptor to look for.
//RETURNS:	-Socket*: The socket found or NULL.
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Searches for a socket by its file descriptor.
//		Returns NULL if it didn't find it.
//------------------------------------------------------------------------------
Socket* SockManager::get_socket(int fd)
{
   SockIter iter(&_list);
   Socket* sock;

   iter.reset();
   for(sock = iter.next();sock != NULL;sock = iter.next())
      if(sock->get_fd() == fd)
         return sock;
   return NULL;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::get_socket
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	Socket* SockManager::get_socket(unsigned char id)
//		-id: the ID to look for.
//RETURNS:	-Socket*: The socket found or NULL.
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Searches for a socket by its ID number.
//		Returns NULL if it didn't find it.
//------------------------------------------------------------------------------
Socket* SockManager::get_socket(unsigned char id)
{
   SockIter iter(&_list);
   Socket* sock;

   iter.reset();
   for(sock = iter.next();sock != NULL;sock = iter.next())
      if(sock->get_id() == id)
         return sock;
   return NULL;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::get_socket
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	Socket* SockManager::get_socket(XtInputId id)
//		-id: The input id registered with Xt
//RETURNS:	-Socket*: The socket found or NULL.
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Searches for a socket by its X input id.
//		Returns NULL if it didn't find it.
//------------------------------------------------------------------------------
Socket* SockManager::get_socket(XtInputId id)
{
   SockIter iter(&_list);
   Socket* sock;

   iter.reset();
   for(sock = iter.next();sock != NULL;sock = iter.next())
      if(sock->get_input_id() == id)
         return sock;
   return NULL;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::add
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockManager::add(Socket* sock)
//		-sock: the socket to add
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Adds a Socket object to this SockManager.
//------------------------------------------------------------------------------
void SockManager::add(Socket* sock)
{
   _list.add(sock);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::new_server
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockManager::new_server()
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Finds the socket with the lowest ID and assumes it is the
//		server.  This function is called when the connection to the
//		server is lost, and another client must assume the
//		responsibility of the server.
//------------------------------------------------------------------------------
void SockManager::new_server()
{
   SockIter iter(&_list);			// to step through socket list
   Socket* sock = NULL;				// temp socket
   unsigned char id = 0;			// holds id number

   // socket with the lowest id is the new server

   // start with my id
   _server_id = _my_id;

   // now look for lower numbers in the socket list
   for(sock = iter.next();sock != NULL;sock = iter.next())
   {
      id = sock->get_id();
      if(id < _server_id && id > 0)
         _server_id = id;
   }

   if(_server_id == _my_id)			// we are the new server
      _is_server = TRUE;
   else						// someone else is
   {
      sock = get_socket(_server_id);
      _is_server = FALSE;
      _server_ip = sock->get_ip();
   }
}


//------------------------------------------------------------------------------
//FUNCTION:	SockManager::remove
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockManager::remove(Socket* sock, int force)
//		-sock: the socket to remove
//		-force: boolean value. If true, don't send a disconnect packet.
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Removes this socket from the SockManager.  Normally it will send
//		a disconnect packet to the socket first, unless force is set to
//		TRUE.
//------------------------------------------------------------------------------
void SockManager::remove(Socket* sock, int force)
{
   unsigned char id = sock->get_id();		// to check for server

   // send disconnect packet if they are connected and this is not a force remove
   if(sock->is_connected() && !force)
      send_packet(sock, B_OP_DISCONNECT, NULL, 0);
   _list.remove(sock);

   // if they were the server, find a new server
   if(id = _server_id)
      new_server();

   // if there's no more connected, just go back to default state
   if(num_connected() == 0)
   {
      _is_server = FALSE;
      _server_id = 0;
      _server_ip = 0;
      _multi = FALSE;
      _my_id = 0;
   }
}


//******************************************************************************
//******************************************************************************

//------------------------------------------------------------------------------
//FUNCTION:	SockList::~SockList (destructor)
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	SockList::~SockList()
//RETURNS:	none
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	deletes all sockets in this socklist.
//------------------------------------------------------------------------------
SockList::~SockList()
{
   for(SockNode* curr = _head;curr != NULL;curr = curr->_next)
      delete curr->_sock;
}


//------------------------------------------------------------------------------
//FUNCTION:	SockList::add
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockList::add(Socket* sock)
//		-sock: the socket to add
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Adds a socket to this list.
//------------------------------------------------------------------------------
void SockList::add(Socket* sock)
{
   SockNode* node;			// holds the current node

   _size++;

   // are we the first to be added?
   if(_head == NULL)
   {
      _head = new SockNode(sock, NULL, NULL);
      return;
   }

   // otherwise find the end of the list and add to there
   for(node = _head;node->_next != NULL;node = node->_next)
      ;
   node->_next = new SockNode(sock, node, NULL);
}


//------------------------------------------------------------------------------
//FUNCTION:	SockList::remove
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	void SockList::remove(Socket* sock)
//		-sock: the socket to remove
//RETURNS:	void
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	removes a socket from this list.
//------------------------------------------------------------------------------
void SockList::remove(Socket* sock)
{
   // let's find that socket
   for(SockNode* curr = _head;curr!= NULL;curr = curr->_next)
   {
      // found it?
      if(curr->_sock == sock)
      {
         // now we do the SockNode shuffle to remove the socket's SockNode.
         if(curr->_prev != NULL)
            curr->_prev->_next = curr->_next;
         else _head = curr->_next;
         if(curr->_next != NULL)
            curr->_next->_prev = curr->_prev;
         delete sock;
         if(--_size <= 0)
            _head = NULL;
      }
   }
}


//******************************************************************************
//******************************************************************************

//------------------------------------------------------------------------------
//FUNCTION:	SockIter::next
//DATE:		May 9, 97
//REVISION:	
//DESIGNER:	Karl Stenerud
//PROGRAMMER:	Karl Stenerud
//INTERFACE:	Socket* SockIter::next()
//RETURNS:	Socket*: the next socket in the attached list.
//DEPENDENCIES: none
//PRECONDITION: none
//NOTES:	Returns the next socket in the attached list or NULL if we're at
//		the end of the list.
//------------------------------------------------------------------------------
Socket* SockIter::next()
{
   SockNode* node = _curr;		// holds the current socket node

   if(_curr != NULL)
   {
      // move the SockList's internal current pointer ahead
      _curr = _curr->_next;
      // return the previously saved node's socket
      return node->_sock;
   }
   return NULL;
}

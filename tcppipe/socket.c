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


#include "socket.h"
#include "ctlproto.h"

WSADATA Socket::_wsadata;     // used to startup/shutdown winsock
int Socket::_count = 0;       // counts the socket objects so we know when
                              // to startup or shutdown winsock

//==============================================================================
// name:   Socket::Socket (constructor)
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Handles the nitty gritty of socket connections & manipulation.
//         I've stripped this one down a bit to reflect the needs of this
//         particular application.
// Inputs: hWnd - handle to the window of the controlling application
//         wMsg - the message to send to the windows callback
// Output: none
Socket::Socket(HWND hWnd, unsigned int wMsg)
: _connected(false), _wmsg(wMsg), _hwnd(hWnd)
{
   if(Socket::_count <= 0)            // start up winsock if we're the first one
   {
      Socket::_count = 0;
      WSAStartup(0x101, &_wsadata);
   }
   Socket::_count++;
}


//==============================================================================
// name:   Socket::~Socket (destructor)
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   kill this socket and shutdown winsock if necessary
// Inputs: none
// Output: none
Socket::~Socket()
{
   disconnect();
   if(--Socket::_count <= 0)         // shutdown winsock, if necessary
   {
      WSACleanup();
      Socket::_count = 0;
   }
}

//==============================================================================
// name:   Socket::disconnect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   disconnect a socket
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool Socket::disconnect()
{
   // don't close an already closed socket
   if(!_connected)
      return false;

   ::shutdown(_fd,2);
   ::closesocket(_fd);
   _connected = 0;

   return true;
}


//============================================================================
//============================================================================


//==============================================================================
// name:   TCPSocket::enable_asyncselect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Enable WSAAsyncSelect to act on this socket.
// Inputs: void
// Output: the result of WSAAsyncSelect()
// Notes:  There's a bit of anomilous behavior with WSAAsyncSelect.
//         It seems that Microsoft decided to have WinSock actually
//         interrupt the WndProc it's supposed to be working with,
//         and manually call the WndProc function with its lParam and
//         wParam.
// I'll demonstrate with an example:
// Suppose you have a protocol that involves reading variable length strings
// across a TCP socket.  Let's say you pass one byte representing the length
// of the string, and then the string itself.  So now on the receiving end
// when acting on WM_COMMAND for WSAAsyncSelect the code would look a bit
// like this:
//
// 01 #define DATA_IN_SOCKET 100
//    ...
// 02 Socket* sock;
//    ...
// 03 int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
//                         LPTSTR lpCmdLine, int nCmdShow)
// 04 {
//       ...
// 05    sock = new Socket(hWnd, DATA_IN_SOCKET); // send DATA_IN_SOCKET on read
//       ...
// 06 }
//
// 07 LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
//                             LPARAM lParam)
// 08 {
//        ...
// 09    switch( uMsg )
// 10    {
// 11       case WM_COMMAND:
// 12          switch(LOWORD(wParam))
// 13          {
// 14             unsigned char len = 0;
// 15             char string[257];     // 256 + 1 for null terminate
//                ...
// 16             case DATA_IN_SOCKET:
// 17                sock->read((char*)&len, 1);   // read 1 byte from socket
// 18                sock->read(string, len);      // read len bytes from socket
// 19                string[len] = 0;              // null terminate
// 20                MessageBox(hWnd, string, "We got this", MB_OK);// tell user
// 21                break;
//                ...
// 22          }
// 23          break;
//          ...
// 24    }
// 25 }
//
// At line 17, we read 1 byte from the socket.  At this point, WSAAsyncSelect
// notices that we have read from this socket, but there's still data in there.
// So it goes and interrupts again.  The problem is that it has interrupted its
// own service routine!  We never actually get to line 18 until everything has
// been read from the socket, at which point there's nothing but garbage in our
// buffers.
// One way to solve this is to wrap the read functions in enable and disable
// asyncselect functions.  The other way is to put socket reading and writing in
// a separate thread.  However, you may want to wrap the reads in a thread with
// enable/disable functions to avoid extra read messages from being generated
int TCPSocket::enable_asyncselect()
{
   return WSAAsyncSelect(_fd, _hwnd, _wmsg, FD_ACCEPT | FD_CLOSE | FD_READ);
}

//==============================================================================
// name:   TCPSocket::enable_asyncselect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   disable WSAAsyncSelect on this socket.
// Inputs: void
// Output: the result of WSAAsyncSelect()
// Notes:  see above
int TCPSocket::disable_asyncselect()
{
   return WSAAsyncSelect(_fd, _hwnd, _wmsg, NULL);
}

//==============================================================================
// name:   TCPSocket::connect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   open a socket and connect to a remote site.
// Inputs: address - the address to connect to
//         port    - the port to connect to
// Output: TRUE/FALSE (success/fail)
bool TCPSocket::connect(char* address, unsigned short port)
{
   LPHOSTENT lpstHost;

   if(_connected)
      disconnect();

   // prepare the address structure
   ::memset(&_saddr, 0, sizeof(_saddr));
   _saddr.sin_family = AF_INET;
   _saddr.sin_port = htons(port);

   // translate the address from dotted quad or hostname to something we can use
   if( (_saddr.sin_addr.s_addr = ::inet_addr(address)) == INADDR_NONE )
      if( (lpstHost = ::gethostbyname(address)) != 0 )
          _saddr.sin_addr.s_addr = *((u_long FAR *)(lpstHost->h_addr));
      else
         return false;

   // now make our socket and connect
   if ((_fd = ::socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
      return false;

   if (::connect(_fd, (LPSOCKADDR)&_saddr, sizeof(_saddr)) == SOCKET_ERROR)
   {
      ::closesocket(_fd);
      return false;
   }

   _connected = true;
   enable_asyncselect();  // let windows tell us when data's ready
   return true;
}

//==============================================================================
// name:   TCPSocket::server
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   open a socket in "server" mode.  This will open a socket on the
//         selected port which will detect incoming connections.
//         Socket::has_input() will return TRUE if there's an incoming
//         connection.
// Inputs: port - the port to listen on
// Output: TRUE/FALSE (success/fail)
// Notes:  when using server(), get_address() and get_port() will return the
//         LOCAL address and port since it would be meaningless to return the
//         "remote" ones.
bool TCPSocket::server(unsigned short port)
{
   if(_connected)
      return false;

   ::memset(&_saddr, 0, sizeof(_saddr));
   _saddr.sin_family = AF_INET;			// address family (internet)
   _saddr.sin_port   = ::htons(port);		// host port number

   // make the socket and bind it to our selected port
   if ((_fd = ::socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
      return false;

   if (::bind(_fd, (struct sockaddr*) &_saddr, sizeof(_saddr)) < 0)
   {
      disconnect();
      return false;
   }

   // max 3 queued connect requests
   ::listen(_fd, 3);

   // we are now connected
   _connected = true;
   enable_asyncselect();
   return true;
}

//==============================================================================
// name:   Socket::accept
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   accept an incoming connection from this socket.
// Inputs: server_fd - the file descriptor of the server socket
// Output: TRUE/FALSE (success/fail)
// Notes:  This one's a bit weird.  What I'm doing is allowing a "server"
//         socket to become the "accepted" socket.
//         I did this because this application will only have one connection.
bool TCPSocket::accept()
{
   static int size = sizeof(_saddr);		// used by accept()
   unsigned int new_fd;							// fd returned by accept

   if(!_connected)
      return false;

   // accept the connection on the new fd
   if( (new_fd = ::accept(_fd, (struct sockaddr*) &_saddr, &size)) == INVALID_SOCKET )
      return false;

   disconnect();					// disconnect the old socket
   _fd = new_fd;              // now assume ownership of the new socket
   _connected = true;

   return true;
}

//==============================================================================
// name:   TCPSocket::read
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   read some data from this socket
// Inputs: buff - the buffer to read into
//         int  - max number of bytes to read
// Output: number of bytes read or -1 if fail (ala stdio read())
int TCPSocket::read(char* buff, int len)
{
   int length;				// number of bytes read

   // make sure we CAN read
   if(!_connected || buff == NULL || len <= 0)
      return 0;

   if((length=::recv(_fd, buff, len, 0)) < 0)
      disconnect();
   return length;
}

//==============================================================================
// name:   TCPSocket::write
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   write some data to this socket
// Inputs: buff - the buffer to write from
//         int  - number of bytes to write
// Output: number of bytes written or -1 if fail (ala stdio read())
int TCPSocket::write(char* buff, int len)
{
   int length;				// number of bytes written

   // make sure we CAN write
   if(!_connected || buff == NULL || len <= 0)
      return 0;

   if((length=::send(_fd, buff, len, 0)) < 0)
      disconnect();
   return length;
}


//============================================================================
//============================================================================

//==============================================================================
// name:   UDPSocket::enable_asyncselect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Enable WSAAsyncSelect to act on this socket.
// Inputs: void
// Output: the result of WSAAsyncSelect()
// Notes:  see TCPSocket::enable_asyncselect for info
int UDPSocket::enable_asyncselect()
{
   return WSAAsyncSelect(_fd, _hwnd, _wmsg, FD_READ);
}

//==============================================================================
// name:   UDPSocket::disable_asyncselect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   disable WSAAsyncSelect on this socket.
// Inputs: void
// Output: the result of WSAAsyncSelect()
// Notes:  see TCPSocket::enable_asyncselect for info
int UDPSocket::disable_asyncselect()
{
   return WSAAsyncSelect(_fd, _hwnd, _wmsg, NULL);
}

//==============================================================================
// name:   UDPSocket::connect
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   open a socket and connect to a remote site.
// Inputs: address - the address to connect to
//         port    - the port to connect to
// Output: TRUE/FALSE (success/fail)
bool UDPSocket::connect(char* address, unsigned short port)
{
   LPHOSTENT lpstHost;

   // fill out an address structure
   ::memset(&_theiraddr, 0, sizeof(_theiraddr));
   _theiraddr.sin_family = AF_INET;
   _theiraddr.sin_port = htons(port);

   // translate the address from dotted quad or hostname to something we can use
   // all subsequent writes will go to this address.
   if( (_theiraddr.sin_addr.s_addr = ::inet_addr(address)) == INADDR_NONE )
      if( (lpstHost = ::gethostbyname(address)) != 0 )
          _theiraddr.sin_addr.s_addr = *((u_long FAR *)(lpstHost->h_addr));
      else
         return false;

   // make a socket if it doesn't exist already.
   // this lets us have a UDP socket in "server" mode, and then
   // have it "connect" to another UDP socket while keeping its original
   // port number.
   if(!_connected)
   {
      ::memset(&_saddr, 0, sizeof(_theiraddr));
      if ((_fd = ::socket(PF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
         return false;
   }

   // Ok, so it's not REALLY connected, but it's the next best thing!

   _connected = true;
   _canwrite = true;
   enable_asyncselect();
   return true;
}


//==============================================================================
// name:   UDPSocket::server
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   open a socket in "server" mode.  For UDP sockets this is simply
//         ensuring the port number of this socket so others can send us data.
// Inputs: port - the port to listen on
// Output: TRUE/FALSE (success/fail)
// Notes:  when using server(), get_address() and get_port() will return the
//         LOCAL address and port since it would be meaningless to return the
//         "remote" ones.
bool UDPSocket::server(unsigned short port)
{
   if(_connected)
      return false;

   ::memset(&_saddr, 0, sizeof(_saddr));
   _saddr.sin_family = AF_INET;			// address family (internet)
   _saddr.sin_port   = ::htons(port);		// host port number

   // make a socket
   if ((_fd = ::socket(PF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
      return false;

   // bind it to our port number
   if (::bind(_fd, (struct sockaddr*) &_saddr, sizeof(_saddr)) < 0)
   {
      _connected = true;
      disconnect();
      return false;
   }

   // we are now connected
   _connected = true;
   _canwrite = false;
   enable_asyncselect();
   return true;
}


//==============================================================================
// name:   UDPSocket::read
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   read some data from this socket
// Inputs: buff - the buffer to read into
//         int  - max number of bytes to read
// Output: number of bytes read or -1 if fail (ala stdio read())
int UDPSocket::read(char* buff, int len)
{
   int length;				// number of bytes read
   struct sockaddr_in trash;
   int addr_len;

   // make sure we CAN read
   if(!_connected || buff == NULL || len <= 0)
      return 0;

   if((length=::recvfrom(_fd, buff, len, 0, (struct sockaddr*) &trash, &addr_len)) < 0)
      disconnect();
   return length;
}


//==============================================================================
// name:   UDPSocket::write
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   write some data to this socket
// Inputs: buff - the buffer to write from
//         int  - number of bytes to write
// Output: number of bytes written or -1 if fail (ala stdio read())
int UDPSocket::write(char* buff, int len)
{
   int length;				// number of bytes written

   // make sure we CAN write
   if(!_connected || !_canwrite || buff == NULL || len <= 0)
      return 0;

   if((length=::sendto(_fd, buff, len, 0, (struct sockaddr*) &_theiraddr, sizeof(_theiraddr))) < 0)
      disconnect();
   return length;
}


//============================================================================
//============================================================================


//==============================================================================
// name:   ControlSock::quit
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send a "quit" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::quit()
{
   char b = CTL_QUIT;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::xon
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send an "xon" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::xon()
{
   char b = CTL_XON;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::xoff
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send an "xoff" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::xoff()
{
   char b = CTL_XOFF;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::dir
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send a "dir" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::dir()
{
   char b = CTL_DIR;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::talk
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send a "talk" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::talk()
{
   char b = CTL_TALK;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::get
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send a "get" message over this socket.
// Inputs: filename - the file to get
// Output: TRUE/FALSE (success/fail)
bool ControlSock::get(char* filename)
{
   unsigned int len = strlen(filename);
   char b[259];

   if(len > 0 && len <= 256)
   {
      memset(b, 0, 259);
      b[0] = CTL_GET;
      b[1] = (unsigned char)len;
      strcpy(b+2, filename);
      return write(b, len+2);
   }
   return false;
}

//==============================================================================
// name:   ControlSock::flist_entry
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send an "flist_entry" message over this socket.
// Inputs: filename - the file to get
// Output: TRUE/FALSE (success/fail)
bool ControlSock::flist_entry(char* filename)
{
   unsigned int len = strlen(filename);
   char b[259];

   if(len > 0 && len <= 256)
   {
      memset(b, 0, 259);
      b[0] = CTL_FLIST_ENTRY;
      b[1] = (unsigned char)len;
      strcpy(b+2, filename);
      return write(b, len+2);
   }
   return false;
}

//==============================================================================
// name:   ControlSock::extract_filename
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Extract a filename from a filename sent by get or flist_entry
// Inputs: fname - buffer to store the filename
// Output: TRUE/FALSE (success/fail)
bool ControlSock::extract_filename(char* fname)
{
   int len_read = -1;
   unsigned char len = 0;

   memset(fname, 0, 257);
   read((char*)&len, 1);
   if(len > 0)
      len_read = read(fname, (int)len);
   if(len_read != (int)len)
      return false;
   return true;
}

//==============================================================================
// name:   ControlSock::invalid_filename
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Send an "invalid_filename" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::invalid_filename()
{
   char b = CTL_INVALID_FILENAME;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::wave_format
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "wave_format" message over this socket.
// Inputs: format - ?
//         size - ?
// Output: TRUE/FALSE (success/fail)
bool ControlSock::wave_format(char* format, long size)
{
//   unsigned int len;
   char b[256];

   if(size > 0 && size <= 256)
   {
      memset(b, 0, size+2);
      b[0] = CTL_WAVE_FORMAT;
//      b[1] = (unsigned char)len;
      memcpy(b+1, format, size);
      return write(b, size+1);
   }
   return false;
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "file end" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::file_end()
{
   char b = CTL_FILE_END;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "pause send file" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::pause_file()
{
   char b = CTL_PAUSE_FILE_SEND;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "resume file sending" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::resume_file()
{
   char b = CTL_RESUME_FILE_SEND;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "stop file sending" message over this socket.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::stop_file()
{
   char b = CTL_STOP_FILE_SEND;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "stop talking" message over this socket. (remote user not
//          talking any more.
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::stop_talking()
{
   char b = CTL_STOP_TALKING;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "start listening" message over this socket. (remote user has
//          started to listen.)
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::start_listening()
{
   char b = CTL_START_LISTENING;
   return write(&b, 1);
}

//==============================================================================
// name:   ControlSock::file_end
// design: Michael Gray
// code:   Michael Gray
// desc:   Send a "stop listening" message over this socket. (remote user not
//          listening any more.)
// Inputs: void
// Output: TRUE/FALSE (success/fail)
bool ControlSock::stop_listening()
{
   char b = CTL_STOP_LISTENING;
   return write(&b, 1);
}



#ifndef _SOCKET_H
#define _SOCKET_H

#define WIN32_LEAN_AND_MEAN
#include <winsock.h>

class Socket
{
public:
                   Socket(HWND hWnd, unsigned int wMsg);
                   ~Socket();
   virtual  bool   connect(char* address, unsigned short port) = 0;
   virtual  bool   server(unsigned short port) = 0;
   virtual  int    read(char* buff, int len) = 0;
   virtual  int    write(char* buff, int len) = 0;
   virtual  int    enable_asyncselect() = 0;
   virtual  int    disable_asyncselect() = 0;
   unsigned short  get_port() {return ntohs(_saddr.sin_port);}
            bool   disconnect();
            SOCKET get_fd() {return _connected ? _fd : -1;}
            bool   is_connected() {return _connected != 0;}
            char*  get_address() {return inet_ntoa(_saddr.sin_addr);}
protected:
          SOCKET       _fd;		   // file descriptor
          bool         _connected;	// flag if connected
   struct sockaddr_in  _saddr;		// holds socket info
          unsigned int _wmsg;       // message to send to windows on input
          HWND         _hwnd;       // window to send it to
   static int          _count;      // counts the sockets
   static WSADATA      _wsadata;    // holds data to shut down winsock

};


class TCPSocket: public Socket
{
public:
   TCPSocket(HWND hWnd, unsigned int wMsg): Socket(hWnd, wMsg) {}
   bool   connect(char* address, unsigned short port);
   bool   server(unsigned short port);
   int    read(char* buff, int len);
   int    write(char* buff, int len);
   int    enable_asyncselect();
   int    disable_asyncselect();
   bool   accept();
private:
};

class UDPSocket: public Socket
{
public:
   UDPSocket(HWND hWnd, unsigned int wMsg): Socket(hWnd, wMsg), _canwrite(false) {}
   bool   connect(char* address, unsigned short port);
   bool   server(unsigned short port);
   int    read(char* buff, int len);
   int    write(char* buff, int len);
   int    enable_asyncselect();
   int    disable_asyncselect();
private:
   struct sockaddr_in  _theiraddr;         // address to send to
   bool                _canwrite;          // gives the OK to send data
};

class ControlSock: public TCPSocket
{
public:
   ControlSock(HWND hWnd, unsigned int wMsg): TCPSocket(hWnd, wMsg) {}
   bool quit();
   bool xon();
   bool xoff();
   bool dir();
   bool talk();
   bool get(char* filename);
   bool invalid_filename();
   bool flist_entry(char* filename);
   bool extract_filename(char* fname);
   bool wave_format(char* format, long size);
   bool file_end();
   bool pause_file();
   bool resume_file();
   bool stop_file();
   bool stop_talking();
   bool start_listening();
   bool stop_listening();
private:
};

#endif


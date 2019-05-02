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

class Socket
{
public:
                  Socket()
                  : _connected(FALSE), _fd(0) {}
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
   unsigned short get_port() {return ntohs(_saddr.sin_port);}
   char*          get_address() {return inet_ntoa(_saddr.sin_addr);}
   unsigned long  get_ip() {return ntohl(_saddr.sin_addr.s_addr);}
private:
   int                _fd;		// file descriptor
   int                _connected;	// flag if connected
   struct sockaddr_in _saddr;		// holds socket info
};

#endif

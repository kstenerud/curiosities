// ============================================================================
// =========================== LICENSING & COPYRIGHT ==========================
// ============================================================================
/*
 *                               Socket Library
 *                              Version 3.1alpha
 *
 * NOTE:  This release of the library is largely untested, and should not be
 *        used in production code!
 *
 *
 * A portable socket library based on the Berkeley Sockets interface.
 * Copyright 1996-2001 Karl Stenerud.  All rights reserved.
 *
 * This code may be freely used for non-commercial purposes as long as this
 * copyright notice remains unaltered in the source code and credit is given
 * to the author (Karl Stenerud).
 *
 * The latest version of this code can be obtained at:
 * http://kstenerud.cjb.net
 */

#include "socket.h"
#include <stdlib.h>
#include <string.h>


// ============================================================================
// ================================ OS DEFINES ================================
// ============================================================================

// Winsock requires some special handling compared to a standard berkeley
// interface.

// Remove me
#define SOCKINTF_WINSOCK

// Default to berkeley if no interface chosen
#ifndef SOCKINTF_BERKELEY
#ifndef SOCKINTF_WINSOCK
#define SOCKINTF_BERKELEY
#endif
#endif

#ifdef SOCKINTF_WINSOCK

#include <winsock.h>

#define SOCK_ERROR SOCKET_ERROR
#define SOCK_INVALID INVALID_SOCKET
#define SOCK_LAST_ERROR() WSAGetLastError()

#ifdef SetPort
#undef SetPort
#endif

#else
#ifdef SOCKINTF_BERKELEY

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define SOCK_ERROR -1
#define SOCK_INVALID -1
#define SOCK_LAST_ERROR() getlasterror();

#endif
#endif



// ============================================================================
// ================================ CLASS DATA ================================
// ============================================================================

int Socket::m_num_sockets = 0;

bool Socket::startup(void)
{
	if(Socket::m_num_sockets == 0)
	{
#ifdef SOCKINTF_WINSOCK
		// Windows requires a call to WSAStartup before doing any socket calls
		WSADATA wsadata;
		int rc = WSAStartup(MAKEWORD( 1, 0 ), &wsadata);
		if(rc != 0)
			return false;
#endif
	}
	Socket::m_num_sockets++;
	return true;
}

// This is called every time a Socket destructor is called
void Socket::shutdown(void)
{
	Socket::m_num_sockets--;
#ifdef SOCKINTF_WINSOCK
	// Windows also requires a call to WSACleanup once we are done
	if(m_num_sockets == 0)
		WSACleanup();
#endif
}


// ============================================================================
// ================================== SOCKET ==================================
// ============================================================================

Socket::Socket()
: LastError(0)
, m_socket(SOCK_INVALID)
, m_remote_port(0)
, m_local_port(0)
{
	Socket::startup();
	m_remote_name = new char[1];
	m_remote_name[0] = 0;
	m_remote_address[0] = 0;

	m_local_name = new char[1];
	m_local_name[0] = 0;
	m_local_address[0] = 0;
}

Socket::~Socket()
{
	Close();
	delete [] m_remote_name;
	delete [] m_local_name;
	Socket::shutdown();
}


void Socket::Close(void)
{
	// Clear all local/remote information
	set_remote_name("");
	m_remote_address[0] = 0;
	m_remote_port = 0;

	set_local_name("");
	m_local_address[0] = 0;
	m_local_port = 0;

	// Close the socket, if neccessary
	if(IsOpen())
	{
		::closesocket(m_socket);
		m_socket = SOCK_INVALID;
	}
}

int Socket::Wait(int event, int wait_msec)
{
	fd_set read_set;
	fd_set write_set;
	fd_set exception_set;
	struct timeval  tval;
	struct timeval* wait_time = NULL;
	int maxfd = m_socket + 1;
	int rc;

	LastError = 0;

	// If wait_msec is not negative, we fill out a timeval structure for
	// select().  Otherwise wait_time will point to NULL, which makes select()
	// wait forever.
	if(wait_msec >= 0)
	{
		wait_time = &tval;
		wait_time->tv_sec = wait_msec / 1000;
		wait_time->tv_usec = (wait_msec % 1000) * 1000;
	}

	// initialize the fd sets.
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&exception_set);

	// In reality, there is no connect event.  It comes in as a read event.
	if(event & (EVENT_READ | EVENT_CONNECT))
		FD_SET(m_socket, &read_set);
	if(event & EVENT_WRITE)
		FD_SET(m_socket, &write_set);
	if(event & EVENT_EXCEPTION)
		FD_SET(m_socket, &exception_set);

	rc = EVENT_NONE;

	// Wait for a socket event
	if(::select(maxfd, &read_set, &write_set, &exception_set, wait_time) != SOCK_ERROR)
	{
		// Note which events occurred
		if(FD_ISSET(m_socket, &read_set))
			rc |= event & (EVENT_READ | EVENT_CONNECT);
		if(FD_ISSET(m_socket, &write_set))
			rc |= EVENT_WRITE;
		if(FD_ISSET(m_socket, &exception_set))
			rc |= EVENT_EXCEPTION;
		return rc;
	}
	LastError = SOCK_LAST_ERROR();
	return -1;
}

int Socket::Read(char* buffer, int length)
{
	int bytes_read;

	LastError = 0;

	if(buffer == NULL || length < 0)
		return -1;

	// Read data, not caring where it's from.
	// In a TCP connection, it will always be from the host we connected to.
	if((bytes_read=::recvfrom(m_socket, buffer, length, 0, NULL, 0)) < 0)
	{
		LastError = SOCK_LAST_ERROR();
		Close();
		return -1;
	}

	return bytes_read;
}

int Socket::Write(char* buffer, int length)
{
	int bytes_written;

	LastError = 0;

	if(buffer == NULL || length < 0)
		return -1;

	// Write data to the host we connected to.
	if((bytes_written=::sendto(m_socket, buffer, length, 0, NULL, 0)) < 0)
	{
		LastError = SOCK_LAST_ERROR();
		Close();
		return -1;
	}

	return bytes_written;
}

int Socket::Write(char* buffer)
{
	return Write(buffer, strlen(buffer));
}

char* Socket::Lookup(char* name)
{
	unsigned long address;
	struct hostent* host_info;

	LastError = 0;

	// Only proceed if it is not a dotted quad address.
	if((address = ::inet_addr(name)) != INADDR_NONE)
	{
		// Resolve the address with DNS
		if((host_info = gethostbyaddr((char*)&address, sizeof(address), PF_INET)) != NULL)
		{
			return host_info->h_name;
		}
	}
	LastError = SOCK_LAST_ERROR();
	return NULL;
}


bool Socket::encode_address(struct sockaddr_in* sockinfo, char* address)
{
	struct hostent* host_info;

	// try to decode dotted quad notation
	if((sockinfo->sin_addr.S_un.S_addr = ::inet_addr(address)) == INADDR_NONE)
	{
		// failing that, look up the name
		if( (host_info = ::gethostbyname(address)) == NULL)
		{
			return false;
		}
		::memcpy(&sockinfo->sin_addr, host_info->h_addr, host_info->h_length);
	}

	return true;
}

void Socket::decode_address(struct sockaddr_in* sockinfo, char* address)
{
	strcpy(address, inet_ntoa(sockinfo->sin_addr));
}

void Socket::encode_port(struct sockaddr_in* sockinfo, unsigned short port)
{
	sockinfo->sin_port   = ::htons(port);
}

unsigned short Socket::decode_port(struct sockaddr_in* sockinfo)
{
	return ::ntohs(sockinfo->sin_port);
}


void Socket::set_remote_name(char* name)
{
	delete [] m_remote_name;
	m_remote_name = new char[strlen(name)+1];
	strcpy(m_remote_name, name);
}

void Socket::set_local_name(char* name)
{
	delete [] m_local_name;
	m_local_name = new char[strlen(name)+1];
	strcpy(m_local_name, name);
}

char* Socket::GetRemoteName(bool lookup)
{
	char* name;

	// Only lookup the name if requested and necessary
	if(m_remote_name[0] == 0 && lookup)
	{
		if((name = Lookup(m_remote_address)) == NULL)
			return NULL;
		set_remote_name(name);
	}
	return m_remote_name;
}

char* Socket::GetRemoteAddress(void)
{
	return m_remote_address;
}

unsigned short Socket::GetRemotePort(void)
{
	return m_remote_port;
}

char* Socket::GetLocalName(bool lookup)
{
	char* name;

	// Only lookup the name if requested and necessary
	if(m_local_name[0] == 0 && lookup)
	{
		if((name = Lookup(m_local_address)) == NULL)
			return NULL;
		set_local_name(name);
	}
	return m_local_name;
}

char* Socket::GetLocalAddress(void)
{
	return m_local_address;
}

unsigned short Socket::GetLocalPort(void)
{
	return m_local_port;
}

unsigned int Socket::GetDescriptor(void)
{
	return m_socket;
}

bool Socket::IsOpen(void)
{
	return m_socket != SOCK_INVALID;
}


// ============================================================================
// ================================ TCP SOCKET ================================
// ============================================================================

TCPSocket::TCPSocket()
: Socket()
, m_listen_queue_size(3)
{
}

bool TCPSocket::Listen(unsigned short port)
{
	static int size = sizeof(struct sockaddr_in);
	static int on = 1;
	struct sockaddr_in sockinfo;

	LastError = 0;

	if(IsOpen())
		return false;

	// open socket
	if( (m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != SOCK_INVALID )
	{
		// Make sure SO_REUSEADDR is on
		if(::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on)) != SOCK_ERROR )
		{
			// Bind the socket to the specified port
			::memset(&sockinfo, 0, sizeof(struct sockaddr_in));
			sockinfo.sin_family = AF_INET;
			encode_port(&sockinfo, port);
			if( ::bind(m_socket, (struct sockaddr*)&sockinfo, sizeof(sockinfo)) != SOCK_ERROR )
			{
				// Retrieve information about the socket
				::memset(&sockinfo, 0, sizeof(sockinfo));
				if( ::getsockname(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
				{
					decode_address(&sockinfo, m_local_address);
					m_local_port = decode_port(&sockinfo);

					// Listen for incoming connections
					if(::listen(m_socket, m_listen_queue_size) != SOCK_ERROR)
					{
						return true;
					}
				}
			}
		}
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return false;
}


bool TCPSocket::Answer(Socket& server)
{
	static int size = sizeof(struct sockaddr_in);
	struct sockaddr_in sockinfo;

	LastError = 0;

	if(IsOpen())
		return false;

	// Accept the incoming connection
	::memset(&sockinfo, 0, sizeof(sockinfo));
	if( (m_socket = ::accept(server.GetDescriptor(), (struct sockaddr*)&sockinfo, &size)) != SOCK_INVALID )
	{
		// Get information about the remote connection
		::memset(&sockinfo, 0, sizeof(sockinfo));
		if( ::getpeername(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
		{
			decode_address(&sockinfo, m_remote_address);
			m_remote_port = decode_port(&sockinfo);

			// Get information about the socket
			::memset(&sockinfo, 0, sizeof(sockinfo));
			if( ::getsockname(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
			{
				decode_address(&sockinfo, m_local_address);
				m_local_port = decode_port(&sockinfo);
				return true;
			}
		}
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return false;
}

bool TCPSocket::Connect(char* address, unsigned short port)
{
	static int size = sizeof(struct sockaddr_in);
	struct sockaddr_in sockinfo;

	LastError = 0;

	if(IsOpen())
		return false;

	// prepare socket information
	::memset(&sockinfo, 0, sizeof(sockinfo));
	if(encode_address(&sockinfo, address))
	{
		sockinfo.sin_family = AF_INET;
		encode_port(&sockinfo, port);

		// Create the socket
		if( (m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != SOCK_INVALID)
		{
			// Connect to the remote host
			if(::connect(m_socket, (struct sockaddr*)&sockinfo, sizeof(sockinfo)) != SOCK_ERROR )
			{
				// Get information about the remote connection
				::memset(&sockinfo, 0, sizeof(sockinfo));
				if( ::getpeername(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
				{
					// if address wasn't dotted quad, it's a named address
					if(::inet_addr(address) == INADDR_NONE)
						set_remote_name(address);
					decode_address(&sockinfo, m_remote_address);
					m_remote_port = decode_port(&sockinfo);

					// Get information about the socket
					::memset(&sockinfo, 0, sizeof(sockinfo));
					if( ::getsockname(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
					{
						decode_address(&sockinfo, m_local_address);
						m_local_port = decode_port(&sockinfo);
						return true;
					}
				}
			}
		}
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return false;
}

void TCPSocket::SetListenQueueSize(int qsize)
{
	if(!IsOpen())
		m_listen_queue_size = qsize;
}

int TCPSocket::GetListenQueueSize(void)
{
	return m_listen_queue_size;
}


// ============================================================================
// ================================ UDP SOCKET ================================
// ============================================================================

bool UDPSocket::Listen(unsigned short port)
{
	static int size = sizeof(struct sockaddr_in);
	static int on = 1;		// used by setsockopt() to turn something "on"
	struct sockaddr_in sockinfo;

	LastError = 0;

	if(IsOpen())
		return false;

	// open socket
	if( (m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != SOCK_INVALID )
	{
		// Make sure SO_REUSEADDR is on
		if(::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on)) != SOCK_ERROR )
		{
			// Bind the socket to the specified port
			::memset(&sockinfo, 0, sizeof(struct sockaddr_in));
			sockinfo.sin_family = AF_INET;
			encode_port(&sockinfo, port);
			if( ::bind(m_socket, (struct sockaddr*)&sockinfo, sizeof(sockinfo)) != SOCK_ERROR )
			{
				// Retrieve information about the socket
				::memset(&sockinfo, 0, sizeof(sockinfo));
				if( ::getsockname(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
				{
					decode_address(&sockinfo, m_local_address);
					m_local_port = decode_port(&sockinfo);

					return true;
				}
			}
		}
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return false;
}


bool UDPSocket::Connect(char* address, unsigned short port)
{
	static int size = sizeof(struct sockaddr_in);
	struct sockaddr_in sockinfo;

	LastError = 0;

	if(IsOpen())
		return false;

	// prepare socket information
	::memset(&sockinfo, 0, sizeof(sockinfo));
	if(encode_address(&sockinfo, address))
	{
		sockinfo.sin_family = AF_INET;
		encode_port(&sockinfo, port);
		decode_address(&sockinfo, address);

		// Create the socket
		if( (m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != SOCK_INVALID)
		{
			// Connect to the remote host
			if(::connect(m_socket, (struct sockaddr*)&sockinfo, sizeof(sockinfo)) != SOCK_ERROR )
			{
				// Get information about the remote connection
				::memset(&sockinfo, 0, sizeof(sockinfo));
				if( ::getpeername(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
				{
					// if address wasn't dotted quad, it's a named address
					if(::inet_addr(address) == INADDR_NONE)
						set_remote_name(address);
					decode_address(&sockinfo, m_remote_address);
					m_remote_port = decode_port(&sockinfo);

					// Get information about the socket
					::memset(&sockinfo, 0, sizeof(sockinfo));
					if( ::getsockname(m_socket, (struct sockaddr*)&sockinfo, &size) != SOCK_ERROR )
					{
						decode_address(&sockinfo, m_local_address);
						m_local_port = decode_port(&sockinfo);
						return true;
					}
				}
			}
		}
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return false;
}


int UDPSocket::ReadFrom(char* buffer, int length, char* address, unsigned short* port)
{
	static int size = sizeof(struct sockaddr_in);
	struct sockaddr_in sockinfo;
	int bytes_read;

	LastError = 0;

	if(buffer == NULL || length < 0)
		return -1;

	// Read data
	::memset(&sockinfo, 0, sizeof(sockinfo));
	if((bytes_read=::recvfrom(m_socket, buffer, length, 0, (struct sockaddr*)&sockinfo, &size)) >= 0)
	{
		// Get information about the sender
		decode_address(&sockinfo, address);
		*port = decode_port(&sockinfo);
		return bytes_read;
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return -1;
}

int UDPSocket::WriteTo(char* buffer, int length, char* address, unsigned short port)
{
	struct sockaddr_in sockinfo;
	int bytes_written;

	LastError = 0;

	if(buffer == NULL || length < 0)
		return -1;

	// Encode information about destination
	::memset(&sockinfo, 0, sizeof(sockinfo));
	if(encode_address(&sockinfo, address))
	{
		encode_port(&sockinfo, port);
		// send data
		if((bytes_written=::sendto(m_socket, buffer, length, 0, (struct sockaddr*)&sockinfo, sizeof(sockinfo))) >= 0)
		{
			return bytes_written;
		}
	}
	LastError = SOCK_LAST_ERROR();
	Close();
	return -1;
}




// ============================================================================
// ============================== SOCKET MANAGER ==============================
// ============================================================================

SocketManager::SocketManager()
{
}

SocketManager::~SocketManager()
{
}

int SocketManager::Wait(int event, int wait_msec)
{
	fd_set read_set;
	fd_set write_set;
	fd_set exception_set;
	unsigned int maxfd = 0;
	struct timeval  tval;
	struct timeval* wait_time = NULL;
	SocketIterator iter;
	int rc;

	m_read_list.erase(m_read_list.begin(), m_read_list.end());
	m_write_list.erase(m_write_list.begin(), m_write_list.end());
	m_exception_list.erase(m_exception_list.begin(), m_exception_list.end());

	// If wait_msec is not negative, we fill out a timeval structure for
	// select().  Otherwise wait_time will point to NULL, which makes select()
	// wait forever.
	if(wait_msec >= 0)
	{
		wait_time = &tval;
		wait_time->tv_sec = wait_msec / 1000;
		wait_time->tv_usec = (wait_msec % 1000) * 1000;
	}

	// initialize the fd sets.
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&exception_set);

	// Add all sockets to fd sets, keeping maxfd updated
	for(iter = m_socket_list.begin();iter != m_socket_list.end();iter++)
	{
		if((*iter)->IsOpen())
		{
			if((*iter)->GetDescriptor() > maxfd)
				maxfd = (*iter)->GetDescriptor();
			// In reality, there is no connect event.  It comes in as a read event.
			if(event & (Socket::EVENT_READ | Socket::EVENT_CONNECT))
				FD_SET((*iter)->GetDescriptor(), &read_set);
			if(event & Socket::EVENT_WRITE)
				FD_SET((*iter)->GetDescriptor(), &write_set);
			if(event & Socket::EVENT_EXCEPTION)
				FD_SET((*iter)->GetDescriptor(), &exception_set);
		}
	}
	maxfd++;

	rc = Socket::EVENT_NONE;

	// Wait for a socket event
	if(select(maxfd, &read_set, &write_set, &exception_set, wait_time) != SOCK_ERROR)
	{
		// Add sockets to event lists based on their pending events
		for(iter = m_socket_list.begin();iter != m_socket_list.end();iter++)
		{
			if((*iter)->IsOpen())
			{
				if(FD_ISSET((*iter)->GetDescriptor(), &read_set))
				{
					rc |= event & (Socket::EVENT_READ | Socket::EVENT_WRITE);
					m_read_list.insert(m_read_list.end(), *iter);
				}
				if(FD_ISSET((*iter)->GetDescriptor(), &write_set))
				{
					rc |= Socket::EVENT_WRITE;
					m_write_list.insert(m_write_list.end(), *iter);
				}
				if(FD_ISSET((*iter)->GetDescriptor(), &exception_set))
				{
					rc |= Socket::EVENT_EXCEPTION;
					m_exception_list.insert(m_exception_list.end(), *iter);
				}
			}
		}
		return rc;
	}
	LastError = SOCK_LAST_ERROR();
	return -1;
}

void SocketManager::Add(Socket* sock)
{
	m_socket_list.insert(m_socket_list.end(), sock);
}

void SocketManager::Remove(Socket* sock)
{
	m_socket_list.remove(sock);
}

Socket* SocketManager::First(void)
{
	m_socket_iterator = m_socket_list.begin();
	return Next();
}

Socket* SocketManager::Next(void)
{
	Socket* found;

	if(m_socket_iterator == m_socket_list.end())
		return NULL;

	found = *m_socket_iterator;
	m_socket_iterator++;
	return found;
}

Socket* SocketManager::FirstRead(void)
{
	m_read_iterator = m_read_list.begin();
	return NextRead();
}

Socket* SocketManager::NextRead(void)
{
	Socket* found;

	if(m_read_iterator == m_read_list.end())
		return NULL;

	found = *m_read_iterator;
	m_read_iterator++;
	return found;
}

Socket* SocketManager::FirstWrite(void)
{
	m_write_iterator = m_write_list.begin();
	return NextWrite();
}

Socket* SocketManager::NextWrite(void)
{
	Socket* found;

	if(m_write_iterator == m_write_list.end())
		return NULL;

	found = *m_write_iterator;
	m_write_iterator++;
	return found;
}

Socket* SocketManager::FirstException(void)
{
	m_exception_iterator = m_exception_list.begin();
	return NextException();
}

Socket* SocketManager::NextException(void)
{
	Socket* found;

	if(m_exception_iterator == m_exception_list.end())
		return NULL;

	found = *m_exception_iterator;
	m_exception_iterator++;
	return found;
}


// ============================================================================
// ==================================== EOF ===================================
// ============================================================================

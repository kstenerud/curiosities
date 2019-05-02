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

#ifndef HEADER__SOCKET
#define HEADER__SOCKET
#include <list>

// ============================================================================
// ================================== SOCKET ==================================
// ============================================================================

class Socket
{
	// Special definitions
	public:
		// Wait Events
		enum
		{
			EVENT_NONE = 0,      // No event (timed out)
			EVENT_CONNECT = 1,   // Connected to remote host
			EVENT_READ = 2,      // Data received from remote host
			EVENT_WRITE = 4,     // Data can be sent to remote host
			EVENT_EXCEPTION = 8  // A socket exception occurred
		};

	// Constructors / Destructors
	public:
		Socket();
		~Socket();

	// Public Methods
	public:
		/** Listen for a connection on the specified port.
		 *
		 *  @args port: port to listen on.
		 *
		 *  @return: true for success, false for failure.
		 */
		virtual bool Listen(unsigned short port) = 0;

		/** Connect to the specified address and port.
		 *
		 *  @args address: the address to connect to in dotted quad or internet name.
		 *
		 *  @args port: port to connect to.
		 *
		 *  @return: true for success, false for failure.
		 */
		virtual bool Connect(char* address, unsigned short port) = 0;

		/** Close the current connection, if any.
		 *
		 *  @return: void
		 */
		virtual void Close(void);

		/** Wait for an event on this socket.
		 *
		 *  @args event: Events to wait for.  Can be any combination of
		 *               EVENT_CONNECT, EVENT_READ, EVENT_WRITE, or
		 *               EVENT_EXCEPTION.
		 *
		 *  @args wait_msec: time to wait for event in milliseconds.  If -1
		 *                   is specified, it will wait forever.
		 *
		 *  @return: event(s) that occurred, if any.
		 */
		virtual int  Wait(int event, int wait_msec);

		/** Read data from this socket.
		 *
		 *  @args buffer: buffer to store the data in.
		 *
		 *  @args length: length of the buffer.
		 *
		 *  @return: number of bytes read, or -1 if an error occurs.
		 */
		virtual int  Read(char* buffer, int length);

		/** Write data to this socket.
		 *
		 *  @args buffer: buffer containing the data.
		 *
		 *  @args length: length of the data.
		 *
		 *  @return: number of bytes written, or -1 if an error occurs.
		 */
		virtual int  Write(char* buffer, int length);

		/** Write a string to this socket.
		 *
		 *  @args str: Null terminated string.
		 *
		 *  @return: number of bytes written, or -1 if an error occurs.
		 */
		virtual int  Write(char* str);

		/** Perform a DNS lookup.
		 *
		 *  @args name: Internet name of a host.
		 *
		 *  @return: pointer to dotted quad address.
		 */
		virtual char* Lookup(char* name);

	// Public Access Methods
	public:
		/** Get the remote host's internet name.
		 *
		 *  @args lookup: If true and the name hasn't been resolved before,
		 *                it will perform a DNS lookup.
		 *
		 *  @return: pointer to internet address or NULL if an error occurs.
		 *
		 *  @notes: If the remote address isn't resolved or the socket isn't
		 *          connected, the name will be an empty string.
		 */
		char* GetRemoteName(bool lookup = false);

		/** Get the remote host's dotted quad address.
		 *
		 *  @return: pointer to a dotted quad address.
		 *
		 *  @notes: If the socket isn't connected, the address will be an
		 *          empty string.
		 */
		char* GetRemoteAddress(void);

		/** Get the remote host's port.
		 *
		 *  @return: the port number, or 0 if the socket is not connected
		 *           to a remote host.
		 */
		unsigned short GetRemotePort(void);

		/** Get the local internet name.
		 *
		 *  @args lookup: If true and the name hasn't been resolved before,
		 *                it will perform a DNS lookup.
		 *
		 *  @return: pointer to internet address or NULL if an error occurs.
		 *
		 *  @notes: If the local address isn't resolved or the socket isn't
		 *          connected, the name will be an empty string.
		 */
		char* GetLocalName(bool lookup = false);

		/** Get the local dotted quad address.
		 *
		 *  @return: pointer to a dotted quad address.
		 *
		 *  @notes: If the socket isn't connected, the address will be an
		 *          empty string.
		 */
		char* GetLocalAddress(void);

		/** Get the local port.
		 *
		 *  @return: the port number, or 0 if the socket is not connected
		 *           to a remote host.
		 */
		unsigned short GetLocalPort(void);

		/** Check if the socket is open
		 *
		 *  @return: True if the socket is listening or connected to a remote
		 *           host.
		 */
		bool IsOpen(void);

		/** Get the socket descriptor
		 *
		 *  @return: The socket descriptor being used by this socket object.
		 */
		unsigned int GetDescriptor(void);

	// Public Members
	public:
		// The last error that occurred when performing a socket call
		// to the OS.
		int LastError;



	// Internal Utility Methods
	protected:
		/** Encode an address into a sockaddr_in structure.
		 *
		 *  @args sockinfo: Structure to fill in.
		 *
		 *  @args address: Address to encode.
		 *
		 *  @return: True if successful.
		 */
		bool encode_address(struct sockaddr_in* sockinfo, char* address);

		/** Decode an address from a sockaddr_in structure.
		 *
		 *  @args sockinfo: Structure to extract from.
		 *
		 *  @args address: buffer to store dotted quad address (must be 16
		 *                 bytes!).
		 */
		void decode_address(struct sockaddr_in* sockinfo, char* address);

		/** Encode a port number into a sockaddr_in structure.
		 *
		 *  @args sockinfo: Structure to fill in.
		 *
		 *  @args port: Port number to encode.
		 */
		void encode_port(struct sockaddr_in* sockinfo, unsigned short port);

		/** Decode a port number from a sockaddr_in structure.
		 *
		 *  @args sockinfo: Structure to extract from.
		 *
		 *  @return: port number.
		 */
		unsigned short decode_port(struct sockaddr_in* sockinfo);

		void set_remote_name(char* name);
		void set_local_name(char* name);

	// Internal Members
	protected:
		// OS-level socket descriptor
		unsigned int m_socket;

		// Internet name of remote host
		char* m_remote_name;

		// Dotted quad address of remote host
		char m_remote_address[16];

		// Port number we are attached to on remote host
		unsigned short m_remote_port;

		// Internet name of local machine
		char* m_local_name;

		// Dotted quad address of local machine
		char m_local_address[16];

		// Port number this socket is bound to
		unsigned short m_local_port;

	// Static Methods & Members
	private:
		// Called whenever a socket object is instantiated
		static bool startup(void);

		// Called whenever a socket object is destroyed
		static void shutdown(void);

		// Keeps track of the number of socket objects in existence
		static int m_num_sockets;
};



// ============================================================================
// ================================ TCP SOCKET ================================
// ============================================================================

class TCPSocket: public Socket
{
	// Constructors / Destructors
	public:
		TCPSocket();

	// Public Methods
	public:

		/** Listen for a connection on the specified port.
		 *
		 *  @args port: port to listen on.
		 *
		 *  @return: true for success, false for failure.
		 */
		bool Listen(unsigned short port);

		/** Answer a connection request on the specified socket.
		 *
		 *  @args server: listening socket that has a pending connection.
		 *
		 *  @return: true for success, false for failure.
		 */
		bool Answer(Socket& server);

		/** Answer a connection request on the specified socket.
		 *
		 *  @args server: listening socket that has a pending connection.
		 *
		 *  @return: true for success, false for failure.
		 */
		bool Answer(Socket* server) {return Answer(*server);}

		/** Connect to the specified address and port.
		 *
		 *  @args address: the address to connect to in dotted quad or internet name.
		 *
		 *  @args port: port to connect to.
		 *
		 *  @return: true for success, false for failure.
		 */
		bool Connect(char* address, unsigned short port);

	// Public Access Methods
	public:
		void SetListenQueueSize(int qsize);
		int GetListenQueueSize(void);


	// Internal Members
	protected:
		// The number of queued connect requests allowed when the socket
		// is in listen mode.
		int m_listen_queue_size;
};



// ============================================================================
// ================================ UDP SOCKET ================================
// ============================================================================

class UDPSocket: public Socket
{
	// Public Methods
	public:
		/** Listen for a connection on the specified port.
		 *
		 *  @args port: port to listen on.
		 *
		 *  @return: true for success, false for failure.
		 */
		bool Listen(unsigned short port);

		/** Connect to the specified address and port.
		 *
		 *  @args address: the address to connect to in dotted quad or internet name.
		 *
		 *  @args port: port to connect to.
		 *
		 *  @return: true for success, false for failure.
		 */
		bool Connect(char* address, unsigned short port);

		/** Read data from this socket, saving information about who sent it.
		 *
		 *  @args buffer: buffer to store the data in.
		 *
		 *  @args length: length of the buffer.
		 *
		 *  @args address: pointer to 16 byte buffer that will be filled
		 *                 with the dotted quad address of the sender.
		 *
		 *  @args port: pointer to the location where the port of the sender
		 *              will be stored.
		 *
		 *  @return: number of bytes read, or -1 if an error occurs.
		 */
		int  ReadFrom(char* buffer, int length, char* address, unsigned short* port);

		/** Write data to a specific address/port.
		 *
		 *  @args buffer: buffer containing the data.
		 *
		 *  @args length: length of the data.
		 *
		 *  @args address: Dotted quad address of the recipient of this packet.
		 *
		 *  @args port: Port number of the recipient of this packet.
		 *
		 *  @return: number of bytes written, or -1 if an error occurs.
		 */
		int  WriteTo(char* buffer, int length, char* address, unsigned short port);
};



// ============================================================================
// ============================== SOCKET MANAGER ==============================
// ============================================================================

class SocketManager
{
	// Constructors / Destructors
	public:
		SocketManager();
		~SocketManager();

	// Public Methods
	public:
		/** Add a socket to the manager.
		 *
		 *  @args sock: Socket to add
		 */
		void Add(Socket* sock);

		/** Remove a socket from the manager.
		 *
		 *  @args sock: Socket to remove
		 */
		void Remove(Socket* sock);

		/** Wait for an event on any of the managed sockets.
		 *
		 *  @args event: Events to wait for.  Can be any combination of
		 *               EVENT_CONNECT, EVENT_READ, EVENT_WRITE, or
		 *               EVENT_EXCEPTION.
		 *
		 *  @args wait_msec: time to wait for event in milliseconds.  If -1
		 *                   is specified, it will wait forever.
		 *
		 *  @return: event(s) that occurred, if any.
		 */
		int Wait(int event, int wait_msec);

	// Public Access Methods
	public:
		/** Get the first managed socket.
		 *  This method will reset the internal increment for this list.
		 *
		 *  @return: the first managed socket or NULL if there are none.
		 */
		Socket* First(void);

		/** Get the next managed socket.
		 *  This method will internally increment, sending the next socket
		 *  in its internal list.
		 *
		 *  @return: the next managed socket or NULL if there are no more.
		 */
		Socket* Next(void);

		/** Get the first socket with a pending connection request.
		 *  This method will reset the internal increment for this list.
		 *  This list is reset every time Wait() is called and is only
		 *  populated if EVENT_CONNECT is specified.
		 *
		 *  @return: the first managed socket or NULL if there are none.
		 */
		Socket* FirstConnect(void) {return FirstRead();}

		/** Get the next socket with a pending connection request.
		 *  This method will internally increment, sending the next socket
		 *  in its internal list.
		 *
		 *  @return: the next managed socket or NULL if there are no more.
		 */
		Socket* NextConnect(void) {return NextRead();}

		/** Get the first socket with data ready to be read.
		 *  This method will reset the internal increment for this list.
		 *  This list is reset every time Wait() is called and is only
		 *  populated if EVENT_READ is specified.
		 *
		 *  @return: the first managed socket or NULL if there are none.
		 */
		Socket* FirstRead(void);

		/** Get the next socket with data ready to be read.
		 *  This method will internally increment, sending the next socket
		 *  in its internal list.
		 *
		 *  @return: the next managed socket or NULL if there are no more.
		 */
		Socket* NextRead(void);

		/** Get the first socket that is ready to have data written to it.
		 *  This method will reset the internal increment for this list.
		 *  This list is reset every time Wait() is called and is only
		 *  populated if EVENT_WRITE is specified.
		 *
		 *  @return: the first managed socket or NULL if there are none.
		 */
		Socket* FirstWrite(void);

		/** Get the next socket that is ready to have data written to it.
		 *  This method will internally increment, sending the next socket
		 *  in its internal list.
		 *
		 *  @return: the next managed socket or NULL if there are no more.
		 */
		Socket* NextWrite(void);

		/** Get the first socket that is in an exceptional state.
		 *  This method will reset the internal increment for this list.
		 *  This list is reset every time Wait() is called and is only
		 *  populated if EVENT_EXCEPTION is specified.
		 *
		 *  @return: the first managed socket or NULL if there are none.
		 */
		Socket* FirstException(void);

		/** Get the next socket that is in an exceptional state.
		 *  This method will internally increment, sending the next socket
		 *  in its internal list.
		 *
		 *  @return: the next managed socket or NULL if there are no more.
		 */
		Socket* NextException(void);

	// Public Members
	public:
		// The last error that occurred when performing a socket call
		// to the OS.
		int LastError;


	// Internal Datatypes
	protected:
		typedef std::list<Socket*> SocketList;
		typedef SocketList::iterator SocketIterator;

	// Internal Members
	protected:
		SocketList m_socket_list;
		SocketList m_read_list;
		SocketList m_write_list;
		SocketList m_exception_list;

		SocketIterator m_socket_iterator;
		SocketIterator m_read_iterator;
		SocketIterator m_write_iterator;
		SocketIterator m_exception_iterator;
};

#endif /* HEADER__SOCKET */

// ============================================================================
// ==================================== EOF ===================================
// ============================================================================

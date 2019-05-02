#include "socklist.h"
#include "player.h"

extern PlayerList Plist;

Socket::~Socket()
{
   Player* plr = Plist.find(_plr_id);	// find the player attached to me
   if(plr != NULL)
      Plist.remove(plr);		// finish him!
   disconnect();			// bye bye!
}

Player* Socket::get_player()
{
   return Plist.find(_plr_id);	// just forward it
}

Socket* Socket::add_to_list(Socket* sock)
{
   Socket* temp = sock;

   if(this == NULL)		// null checking to avoid a nasty crash
      return NULL;

   // make sure we're at the end of the list
   if(temp != NULL)
   {
      for(;temp->next() != NULL;temp = temp->next())
         ;
      temp->_next = this;
   }

   // then set up the pointers.
   _prev = temp;
   _next = NULL;

   return this;
}

Socket* Socket::remove_from_list()
{
   Socket* temp;

   if(this == NULL)
      return NULL;

   // set the pointers of any adjacent Socket objects.
   if(_prev != NULL)
      _prev->_next = _next;
   if(_next != NULL)
      _next->_prev = _prev;

   if( (temp=_next) == NULL)
      temp = _prev;

   // delete this socket.
   delete this;

   // this is the next valid Socket object in the list.
   return temp;
}


int Socket::connect(int serv_sock)
{
   static int size = sizeof(struct sockaddr_in);

   if(_connected)		// don't connect again!
      return FALSE;

   _connected = TRUE;
   _server_socket = serv_sock;

   // accept the connection pending on the server socket.
   if( (_socket = ::accept(serv_sock, (struct sockaddr*) &_sockaddr_in
                          , &size)) < 0 )
   {
      perror("Socket::connect: ::accept");
      disconnect();
      return FALSE;
   }

   return TRUE;
}

int Socket::disconnect()
{
   if(!_connected)		// don't disconnect again!
      return FALSE;

   close(_socket);		// close the socket
   _socket = 0;			// reset everything
   _server_socket = 0;
   _connected = FALSE;
   memset(&_sockaddr_in, 0, sizeof(struct sockaddr_in));

   return TRUE;
}

int Socket::add_input()
{
   char* buff = new char[_max_input_len+1];		// input from read()
   int len = ::read(_socket, buff, _max_input_len);	// bytes read

   if(len < 1)				// error in read
   {
      delete [] buff;
      return FALSE;
   }

   buff[len] = 0;			// null terminate

   _buffer += buff;			// add to buffer
   delete [] buff;

   if(_buffer.is_overflow())		// check for overflow
   {
      writestring("Buffer overflow!\n");
      return FALSE;
   }

   // put a command in the furrent command buffer, if possible.
   if(_current_cmd.is_empty() && _buffer.has_newline())
      _current_cmd = _buffer.extract_line();

   return TRUE;
}

//==============================================================================

Socket_list::Socket_list(int port, int maxlen)
: _start(NULL), _end(NULL), _current(NULL), _current_in(NULL)
,_length(0), _max_length(maxlen < FD_SETSIZE ? maxlen : FD_SETSIZE)
{
   struct sockaddr_in saddr;
   struct hostent*    pHost;
   char               hname[101];
   int                on = 1;
   int                off = 0;

   ::gethostname(hname, 100);			// who are we?

   if( (pHost = ::gethostbyname(hname)) == NULL) // fill out host structure
   {
      perror("init_server_socket: gethostbyname");
      exit(1);
   }

   ::memset(&saddr, 0, sizeof(saddr));
   saddr.sin_family = pHost->h_addrtype;	// address family (AF_INET)
   saddr.sin_port   = ::htons(port);		// host port number

   // open socket
   if( (_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
   {
      perror("init_server_socket: socket");
      exit(1);
   }

   // options
   if( setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &on,
       sizeof(on)) < 0 )
   {
       perror( "init_server_socket: setsockopt SO_REUSEADDR" );
       close(_server_fd);
       exit( 1 );
   }

   // bind host and port to socket
   if( bind(_server_fd,(struct sockaddr*)&saddr,sizeof(saddr)) < 0 )
   {
      perror("init_server_socket: bind");
      close(_server_fd);
      exit(1);
   }

   listen(_server_fd, 3);		//allow up to 3 queued connects

   _max_fd = _server_fd;
   FD_ZERO(&_in_set);
   FD_ZERO(&_out_set);
   FD_ZERO(&_exc_set);
}


void Socket_list::gather_input()
{
   Socket* temp_node;

   for(temp_node = _start;temp_node != NULL;temp_node = temp_node->next())
   {
      // Get input from the sockets.  delete them if anything goes wrong.
      if(FD_ISSET(temp_node->get_sockfd(), &_in_set))
         if(!temp_node->add_input())
            if( (temp_node=remove(temp_node)) == NULL )
               break;

      // Just delete any exceptions.
      if(FD_ISSET(temp_node->get_sockfd(), &_exc_set))
         if( (temp_node=remove(temp_node)) == NULL )
            break;
   }
}

Socket* Socket_list::next_input()
{
   Socket* temp_node;

   // start where we left off last time.
   for(;_current_in != NULL;_current_in = _current_in->next())
   {
      // if there's data ready, return this node.
      if(_current_in->data_ready())
      {
         temp_node = _current_in;
         _current_in = _current_in->next();
         return temp_node;
      }
   }
   // reset the current position pointer.
   _current_in = _start;
   return NULL;
}

int Socket_list::wait_for_input(int wait)
{
   Socket*      temp_node;

   // Here I implement a little hack to allow timeouts or no timeouts.
   // select() takes a pointer to a struct timeval as its timeout value.
   // if the timeout pointer is NULL, select() won't timeout.
   // I use a comcrete timeval structure and a pointer which is initialized
   // to NULL.
   struct timeval  tval;
   struct timeval* wait_time = NULL;

   // if the wait value was less than 0, wait_time remains NULL and select
   // won't timeout.  Otherwise wait determines how many seconds to wait.
   if(wait >= 0)
   {
      wait_time = &tval;
      wait_time->tv_sec = wait;
      wait_time->tv_usec = 0;
   }

   // initialize the fd sets.
   FD_ZERO(&_in_set);
   FD_ZERO(&_out_set);
   FD_ZERO(&_exc_set);
   FD_SET(_server_fd, &_in_set);
   for(temp_node = _start;temp_node != NULL;temp_node = temp_node->next())
   {
      FD_SET(temp_node->get_sockfd(), &_in_set);
//      FD_SET(temp_node->get_sockfd(), &_out_set);
      FD_SET(temp_node->get_sockfd(), &_exc_set);
   }
   return select(_max_fd+1, &_in_set, &_out_set, &_exc_set, wait_time);
}


Socket* Socket_list::add_new_connection()
{
   Socket* temp_sock;

   // If the server is set, there's a new connection.
   if(FD_ISSET(_server_fd, &_in_set))
   {
      // make a new socket
      temp_sock = new Socket();

      // try to open a connection
      if(temp_sock->connect(_server_fd))
      {
         if(add(temp_sock))
            return temp_sock;
         else
            temp_sock->writestring("Too many connections. Try again later\n");
      }
      // if we couldn't open a connection, delete the socket.
      delete temp_sock;
   }
   return NULL;
}


Socket* Socket_list::first()
{
   // just set the current pointer.
   _current = _start;
   return _current;
}

Socket* Socket_list::next()
{
   // move forward one node.
   _current = _current->next();
   return _current;
}

Socket_list::~Socket_list()
{
   Socket* temp;

   while(_end != NULL)
   {
      temp = _end->prev();
      delete _end;
      _end = temp;
   }
   close(_server_fd);
}

int Socket_list::add(Socket* sock)
{
   // get the file descriptor of this socket.
   int new_fd = sock->get_sockfd();

   // make sure there's room.
   if(_length == _max_length)
      return FALSE;

   // first entry?
   if(_start == NULL)
      _end = _start = _current = _current_in = sock;
   else
   {
      sock->add_to_list(_end);
      _end = sock;
   }
   _length++;

   // keep track of the highest valued file descriptor (for the select() call).
   if(new_fd > _max_fd)
      _max_fd = new_fd;

   return TRUE;
}


Socket* Socket_list::remove(Socket* del_node)
{
   int        del_fd  = del_node->get_sockfd();
   Socket*    temp_node;
   Socket*    new_node = del_node->remove_from_list();

   _length--;
   // move any pointers that pointed to the now deleted node.
   if(_start == del_node)
      _start = new_node;
   if(_end == del_node)
      _end = new_node;
   if(_current == del_node)
      _current = new_node;
   if(_current_in == del_node)
      _current_in = new_node;

   // keep track of the highest valued file descriptor.
   if(del_fd == _max_fd)
   {
      _max_fd = _server_fd;
      for(temp_node = _start;temp_node!=NULL;temp_node = temp_node->next())
         if(temp_node->get_sockfd() > _max_fd)
            _max_fd = temp_node->get_sockfd();
   }

   return new_node;
}

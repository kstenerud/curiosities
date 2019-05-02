// terminal.c

#include <stdio.h>
#include "terminal.h"

//------------------------------------------------------------------------------
// Name:   Terminal::open
// Desc:   open a tty device and control it for I/O
// Inputs: device - the path to the device file
//         baud   - the baud rate to use
//         data   - number of data bits to use (5-8)
//         parity - parity to use ('o', 'e', 'n')
//         stop   - stop bits to use (1-2)
// Output: -1 if failure, otherwise 0
int Terminal::open(char* device, int baud, char data, char parity, char stop, int raw)
{
   if(_fd >= 0)					// don't clobber open file
      return -1;

   if( (_fd=::open(device, O_RDWR)) < 0)	// open the device
   {
      ::perror("Terminal::open: open");
      return -1;
   }

   if(!::isatty(_fd))				// make sure is a tty
   {
      ::printf("%s: not a tty\n", device);
      ::close(_fd);
      _fd = -1;
      return -1;
   }

   ::ioctl(_fd, TCGETS, &_orig_termios);	// save old termios
   _baud = baud;
   _data = data;
   _parity = parity;
   _stop = stop;
   _raw = raw;
   if(_update_settings() >= 0)			// set new settings
      return 0;

   printf("Couldn't change settings\n");
   ::close(_fd);
   _fd = -1;
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::manage_terminal
// Desc:   control an already open terminal
// Inputs: fd  - file descriptor of the open terminal
//         raw - boolean value (true means raw mode)
// Output: -1 if failure, otherwise 0
int Terminal::manage_terminal(int fd, int raw)
{
   _fd = fd;
   _managed = 1;

   if(!::isatty(_fd))			// make sure it's a tty
   {
      ::printf("not a tty\n");
      _fd = -1;
      return -1;
   }

   ::ioctl(_fd, TCGETS, &_orig_termios);// get current settings
   _current_termios = _orig_termios;
   _sync_settings();			// sync termios and object settings
   _raw = raw;				// put us in raw mode, if needed
   if(_update_settings() >= 0)
      return 0;

   printf("Couldn't change settings\n");
   _fd = -1;
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::_update_settings (private)
// Desc:   bring the termios structure up to date with the object settings
// Inputs: none
// Output: -1 if failure, otherwise 0
int Terminal::_update_settings()
{
   if(_fd < 0)				// make sure we're valid
      return -1;

   _current_termios = _orig_termios;	// start with clean slate
   if(_raw)
   {
      _current_termios.c_iflag = 0;
      _current_termios.c_lflag = 0;
      _current_termios.c_cc[VMIN] = 1;
      _current_termios.c_cc[VTIME] = 0;
   }

   _current_termios.c_cflag &= ~CBAUD;	// set the baud rate
   switch(_baud)
   {
   case 50:
      _current_termios.c_cflag |= B50;
      break;
   case 75:
      _current_termios.c_cflag |= B75;
      break;
   case 110:
      _current_termios.c_cflag |= B110;
      break;
   case 134:
      _current_termios.c_cflag |= B134;
      break;
   case 150:
      _current_termios.c_cflag |= B150;
      break;
   case 200:
      _current_termios.c_cflag |= B200;
      break;
   case 300:
      _current_termios.c_cflag |= B300;
      break;
   case 600:
      _current_termios.c_cflag |= B600;
      break;
   case 1200:
      _current_termios.c_cflag |= B1200;
      break;
   case 2400:
      _current_termios.c_cflag |= B2400;
      break;
   case 4800:
      _current_termios.c_cflag |= B4800;
      break;
   case 9600:
      _current_termios.c_cflag |= B9600;
      break;
   case 19200:
      _current_termios.c_cflag |= B19200;
      break;
   case 38400:
      _current_termios.c_cflag |= B38400;
      break;
   case 57600:
      _current_termios.c_cflag |= B57600;
      break;
   case 115200:
      _current_termios.c_cflag |= B115200;
      break;
   case 230400:
      _current_termios.c_cflag |= B230400;
      break;
   case 460800:
      _current_termios.c_cflag |= B460800;
      break;
   default:
      return -1;
   }

   _current_termios.c_cflag &= ~CSIZE;	// set the data bits
   switch(_data)
   {
   case 5:
      _current_termios.c_cflag |= CS5;
      break;
   case 6:
      _current_termios.c_cflag |= CS6;
      break;
   case 7:
      _current_termios.c_cflag |= CS7;
      break;
   case 8:
      _current_termios.c_cflag |= CS8;
      break;
   default:
      return -1;
   }

   switch(_stop)			// set the stop bits
   {
   case 1:
      _current_termios.c_cflag &= ~CSTOPB;
      break;
   case 2:
      _current_termios.c_cflag |= CSTOPB;
      break;
   default:
      return -1;
   }

   switch(_parity)			// set the parity
   {
   case 'o':
      _current_termios.c_cflag |= (PARENB | PARODD);
      break;
   case 'e':
      _current_termios.c_cflag |= PARENB;
      _current_termios.c_cflag &= ~PARODD;
      break;
   case 'n':
      _current_termios.c_cflag &= ~(PARENB | PARODD);
      break;
   default:
      return -1;
   }

   _current_termios.c_iflag |= (INPCK | IXON | IXOFF | IGNBRK);
//   _current_termios.c_cflag |= (HUPCL | CRTSCTS);

   return ::ioctl(_fd, TCSETS, &_current_termios);	// update the termios
}


//------------------------------------------------------------------------------
// Name:   Terminal::setbaud
// Desc:   set the baud rate of this terminal
// Inputs: baud - the baud rate to use
// Output: -1 if failure, otherwise 0
int Terminal::setbaud(int baud)
{
   if(_fd < 0)
      return -1;

   int baud_bak = _baud;		// backup old baud rate

   _baud = baud;
   if(_update_settings() > -1)		// try to set it
      return 0;

   _baud = baud_bak;			// otherwise restore old value
   return -1;
}

//------------------------------------------------------------------------------
// Name:   Terminal::setdata
// Desc:   set the data bits of this terminal
// Inputs: data - the bits to use
// Output: -1 if failure, otherwise 0
int Terminal::setdata(char data)
{
   if(_fd < 0)
      return -1;

   char data_bak = _data;		// back up old data bits

   _data = data;
   if(_update_settings() > -1)		// try to set it
      return 0;

   _data = data_bak;			// otherwise restore old value
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::setparity
// Desc:   set the parity of this terminal
// Inputs: parity - parity to use
// Output: -1 if failure, otherwise 0
int Terminal::setparity(char parity)
{
   if(_fd < 0)
      return -1;

   char parity_bak = _parity;		// backup old parity

   _parity = parity;
   if(_update_settings() > -1)		// try to set it
      return 0;

   _parity = parity_bak;		// otherwise restore old value
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::setstop
// Desc:   set the stop bits of this terminal
// Inputs: stop - the stop bits to use
// Output: -1 if failure, otherwise 0
int Terminal::setstop(char stop)
{
   if(_fd < 0)
      return -1;

   char stop_bak = _stop;		// backup old stop bits

   _stop = stop;
   if(_update_settings() > -1)		// try to set it
      return 0;

   _stop = stop_bak;			// otherwise restore old value
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::setraw
// Desc:   set/unset the raw mode of this terminal
// Inputs: raw - TRUE = raw mode, FALSE = cooked mode
// Output: -1 if failure, otherwise 0
int Terminal::setraw(int raw)
{
   if(_fd < 0)
      return -1;

   int raw_bak = _raw;			// backup old raw flag
   if(_fd > -1)
   {
      _raw = raw;
      if(_update_settings() > -1)	// try to set it
         return 0;
   }
   _raw = raw_bak;			// otherwise restore old value
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::setall
// Desc:   set the baud rate, data bits, parity, stop bits, and raw mode of
//         this terminal
// Inputs: baud   - the baud rate to use
//         data   - number of data bits to use (5-8)
//         parity - parity to use ('o', 'e', 'n')
//         stop   - stop bits to use (1-2)
//         raw    - TRUE = raw mode, FALSE = cooked mode
// Output: -1 if failure, otherwise 0
int Terminal::setall(int baud, char data, char parity, char stop, int raw)
{
   int  baud_bak = _baud;		// backup old settings
   char data_bak = _data;
   char parity_bak = _parity;
   char stop_bak = _stop;
   int  raw_bak = _raw;

   if(_fd < 0)
      return -1;

   _baud = baud;			// try to set it
   _data = data;
   _parity = parity;
   _stop = stop;
   _raw = raw;
   if(_update_settings() > -1)
      return 0;

   _baud = baud_bak;			// otherwise restore old settings
   _data = data_bak;
   _parity = parity_bak;
   _stop = stop_bak;
   _raw = raw_bak;
   return -1;
}


//------------------------------------------------------------------------------
// Name:   Terminal::read
// Desc:   read data from this terminal
// Inputs: buffer - stores the read data
//         len    - number of bytes to read
// Output: -1 if failure, otherwise # of bytes read
int Terminal::read(char* buffer, int len)
{
   if(_fd < 0)
      return -1;

   return(::read(_fd, buffer, len));
}

//------------------------------------------------------------------------------
// Name:   Terminal::readchar
// Desc:   read 1 char from this terminal
// Inputs: none
// Output: -1 if failure, otherwise the value read
char Terminal::readchar()
{
   char c;

   if(_fd < 0)
      return 0;

   if(Terminal::read(&c, 1) < 0)
      return 0;
   return c;
}

//------------------------------------------------------------------------------
// Name:   Terminal::wait_and_read
// Desc:   wait for data to become available, then read it
// Inputs: buffer - stores the read data
//         len    - number of bytes to read
//         wait   - number of seconds to wait. (-1 means indefinitely)
// Output: -1 if failure, otherwise # of bytes read
int Terminal::wait_and_read(char* buffer, int len, int wait)
{
   fd_set in_set;			// descriptor sets
   fd_set out_set;
   fd_set exc_set;
   struct timeval tval;			// timer
   struct timeval* tv = NULL;		// if tv = NULL, wait indefinitely

   if(_fd < 0)
      return -1;

   // now if a value less than 0 is passed in as the wait value, we can set up
   // select to wait indefinitely.
   if(wait >= 0)
   {
      tv = &tval;
      tv->tv_sec = wait;
      tv->tv_usec = 0;
   }

   FD_ZERO(&in_set);
   FD_ZERO(&out_set);
   FD_ZERO(&exc_set);

   FD_SET(_fd, &in_set);
   if( ::select(_fd+1, &in_set, &out_set, &exc_set, tv) < 0 )
   {
      ::perror("wait_and_read: select");
      exit(1);
   }
   if(FD_ISSET(_fd, &in_set))			// is data available?
      return Terminal::read(buffer, len);	// read the data.
}

//------------------------------------------------------------------------------
// Name:   Terminal::wait_and_readchar
// Desc:   wait for data to become available, then read 1 char
// Inputs: wait - number of seconds to wait. (-1 means indefinitely)
// Output: -1 if failure, otherwise the value read
char Terminal::wait_and_readchar(int wait)
{
   char c;

   if(_fd < 0)
      return 0;

   if(Terminal::wait_and_read(&c, 1, wait) < 0)
      return 0;
   return c;
}


//------------------------------------------------------------------------------
// Name:   Terminal::write
// Desc:   Write data to this terminal
// Inputs: buffer - data to be written
//         len    - number of bytes to write
// Output: -1 if failure, otherwise # of bytes written
int Terminal::write(char* buffer, int len)
{
   if(_fd < 0)
      return -1;

   return ::write(_fd, buffer, len);
}


//------------------------------------------------------------------------------
// Name:   Terminal::_sync_settings (private)
// Desc:   Adjust the object settings to the actual terminal settings
// Inputs: none
// Output: -1 if failure, otherwise 0
int Terminal::_sync_settings()
{
   struct termios temp;			// holds the terminal settings

   if(_fd < 0)
      return -1;

   ioctl(_fd, TCGETS, &temp);		// get our terminal settings

   // now decode the settings
   switch(temp.c_cflag & CBAUD)		// baud rate
   {
   case B460800:
      _baud = 460800;
      break;
   case B230400:
      _baud = 230400;
      break;
   case B115200:
      _baud = 115200;
      break;
   case B57600:
      _baud = 57600;
      break;
   case B38400:
      _baud = 38400;
      break;
   case B19200:
      _baud = 19200;
      break;
   case B9600:
      _baud = 9600;
      break;
   case B4800:
      _baud = 4800;
      break;
   case B2400:
      _baud = 2400;
      break;
   case B1200:
      _baud = 1200;
      break;
   case B600:
      _baud = 600;
      break;
   case B300:
      _baud = 300;
      break;
   case B200:
      _baud = 200;
      break;
   case B150:
      _baud = 150;
      break;
   case B134:
      _baud = 134;
      break;
   case B110:
      _baud = 110;
      break;
   case B75:
      _baud = 75;
      break;
   case B50:
      _baud = 50;
      break;
   default:
      return -1;
   }

   switch(temp.c_cflag & CSIZE)			// data bits
   {
   case CS5:
      _data = 5;
      break;
   case CS6:
      _data = 6;
      break;
   case CS7:
      _data = 7;
      break;
   case CS8:
      _data = 8;
      break;
   default:
      return -1;
   }

   if(temp.c_cflag & CSTOPB)			// stop bits
      _stop = 2;
   else _stop = 1;

   switch(temp.c_cflag & (PARENB | PARODD))	// parity
   {
   case (PARENB | PARODD):
      _parity = 'o';
      break;
   case PARENB:
      _parity = 'e';
      break;
   default:
      _parity = 'n';
   }
   return 0;
}


//------------------------------------------------------------------------------
// Name:   Terminal::close
// Desc:   close this terminal
// Inputs: none
// Output: -1 if failure, otherwise 0
int Terminal::close()
{
   int r = 0;			// return value

   if(_fd < 0)
      return -1;

   if(_managed)			// don't close a managed terminal, just restore
      restore_settings();
   else if((r=::close(_fd)) < 0)// but do close an opened terminal
      return -1;

   _fd = -1;

   return r;
}


//------------------------------------------------------------------------------
// Name:   Terminal::restore_settings
// Desc:   Restore this terminal's original settings
// Inputs: none
// Output: 0 (unconditional)
int Terminal::restore_settings()
{
   _current_termios = _orig_termios;		// get original termios
   ::ioctl(_fd, TCSETS, &_current_termios);	// set it
   _sync_settings();				// sync it
   return 0;
}


//------------------------------------------------------------------------------
// Name:   Terminal::hangup
// Desc:   hangup the modem.
// Inputs: none
// Output: void
void Terminal::hangup()
{
   struct termios temp = _current_termios;

   temp.c_cflag &= ~CBAUD;			// clear current baud rate
   temp.c_cflag |= B0;				// setting B0 causes hangup
   ::ioctl(_fd, TCSETS, &temp);			// set it
   sleep(1);
   ::ioctl(_fd, TCSETS, &_current_termios);	// restore old settings again

}

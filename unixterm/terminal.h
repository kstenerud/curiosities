#ifndef _TERMINAL_H
#define _TERMINAL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>

class Terminal
{
   public:
      // constructor.  Note that this object does NOTHING until it opens or
      // manages a terminal.
      Terminal(): _fd(-1), _baud(0), _data(0), _stop(0), _parity(0), _raw(0)
                , _managed(0) {}

      // close or restore the terminal upon killing this object
      ~Terminal() {Terminal::close();}

      // open a new terminal
      int open(char* device, int baud, char data, char parity, char stop, int raw);

      // manage an already open terminal
      int manage_terminal(int fd, int raw);

      // restore original settings
      int restore_settings();

      // close this terminal
      int close();

      // change some settings
      int setbaud(int baud);
      int setdata(char data);
      int setparity(char parity);
      int setstop(char stop);
      int setraw(int raw);
      int setall(int baud, char data, char parity, char stop, int raw);

      // read from this terminal
      int read(char* buffer, int len);
      char readchar();

      // wait for data to become available, then read
      int wait_and_read(char* buffer, int len, int wait);
      char wait_and_readchar(int wait);

      // write to this terminal
      int write(char* buffer, int len);
      int writechar(char c) {return Terminal::write(&c, 1);}
      int writestring(char* str) {return Terminal::write(str, strlen(str));}

      // hangup the modem
      void hangup();

   private:
      int            _update_settings();	// bring terminal up to date
      int            _sync_settings();		// bring object up to date
      int            _managed;			// flag: is this a managed term?
      int            _raw;			// flag: are we using raw mode?
      int            _fd;			// file descriptor of terminal
      int            _baud;			// baud rate
      char           _data;			// data bits (5-8)
      char           _parity;			// parity    ('o', 'e', 'n')
      char           _stop;			// stop bits (1-2)
      struct termios _orig_termios;		// original terminal settings
      struct termios _current_termios;		// current terminal settings
};

#endif

// help.h
#ifndef _MY_HELP_H
#define _MY_HELP_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "defaults.h"


class Help
{
   public:
      		Help		();
      		~Help		();
      void	ImmLevel	(int, int);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      int	_level;
      Cstring	_name;
      Cstring	_desc;
      Help	*_next;
};
ifstream& operator>>(ifstream&, Help&);
ofstream& operator<<(ofstream&, Help&);

#endif

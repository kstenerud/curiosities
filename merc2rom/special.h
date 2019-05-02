// special.h

#ifndef _MY_SPECIAL_H
#define _MY_SPECIAL_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "defaults.h"

class Special
{
   public:
      		Special		();
      		~Special	();
      void	BaseVnum	(int, int, int);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      char	_type;
      int	_mobile;
      Cstring	_special;
      Cstring	_comment;
      Special	*_next;
};
ifstream& operator>>(ifstream&, Special&);
ofstream& operator<<(ofstream&, Special&);

#endif

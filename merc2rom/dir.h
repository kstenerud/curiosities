// dir.h

#ifndef _MY_DIR_H
#define _MY_DIR_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "flags.h"
#include "defaults.h"

class Dir
{
   public:
      		Dir		();
      		~Dir		();
      void	BaseVnum	(int, int, int);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      int	_direction;
      Cstring	_desc;
      Cstring	_name;
      Cstring	_state;
      int	_key;
      int	_dest_room;
};

ifstream& operator>>(ifstream&, Dir&);
ofstream& operator<<(ofstream&, Dir&);

#endif

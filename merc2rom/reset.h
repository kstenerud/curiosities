// reset.h

#ifndef _MY_RESET_H
#define _MY_RESET_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "mobile.h"
#include "object.h"
#include "defaults.h"

class Reset
{
   public:
      		Reset		();
      		~Reset		();
      void	BaseVnum	(int, int, int, int, int, int, int);
      void	SetObjLevels	(Mobile*, Object*, int=-1);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      char	_type;
      int	_a;
      int	_b;
      int	_c;
      int	_d;
      Cstring	_comment;
      Reset	*_next;
};
ifstream& operator>>(ifstream&, Reset&);
ofstream& operator<<(ofstream&, Reset&);

#endif

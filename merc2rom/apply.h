// apply.h

#ifndef _MY_APPLY_H
#define _MY_APPLY_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "fileio.h"
#include "defaults.h"

class Apply
{
   public:
      		Apply	();
      		~Apply	();
      ifstream&	Read	(ifstream&);
      ofstream&	Write	(ofstream&);
   private:
      int	_apply;
      int	_value;
      Apply	*_next;
};

ofstream& operator<<(ofstream&, Apply&);
ifstream& operator>>(ofstream&, Apply&);

#endif

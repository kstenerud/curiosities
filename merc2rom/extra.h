// extra.h

#ifndef _MY_EXTRA_H
#define _MY_EXTRA_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "fileio.h"
#include "defaults.h"

class Extra
{
   public:
      		Extra	();
      		~Extra	();
      ifstream&	Read	(ifstream&);
      ofstream&	Write	(ofstream&);
   private:
      Cstring	_name;
      Cstring	_desc;
      Extra	*_next;
};

ifstream& operator>>(ifstream&, Extra&);
ofstream& operator<<(ofstream&, Extra&);

#endif

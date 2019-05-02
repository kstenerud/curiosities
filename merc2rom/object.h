// object.h

#ifndef _MY_OBJECT_H
#define _MY_OBJECT_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "flags.h"
#include "defaults.h"
#include "apply.h"
#include "extra.h"
#include "list.h"

class Object
{
   public:
      Object();
      void	NoChange	();
      void	BaseVnum	(int, int=-1, int=-1);
      int	FindLowVnum	(int=-1);
      int	FindHighVnum	(int=-1);
      Object*	FindVnum	(int);
      void	BaseLevel	(int);
      void	SetLevel	(int);
      void	ListAdd		(List&);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      int	_vnum;
      Cstring	_name;
      Cstring	_short_desc;
      Cstring	_long_desc;
      Cstring	_action;		// merc...
      Cstring	_material;	// rom
      Cstring	_type;
      Cstring	_extra_flags;
      Cstring	_wear_flags;
      Cstring	_v0;
      Cstring	_v1;
      Cstring	_v2;
      Cstring	_v3;
      Cstring	_v4;		// rom
      int	_level;		// rom
      int	_weight;
      int	_cost;
      Apply	*_applies;
      Extra	*_extras;
      int	_nochange;
      Object	*_next;
};
ifstream& operator>>(ifstream&, Object&);
ofstream& operator<<(ofstream&, Object&);

#endif

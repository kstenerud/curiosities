// zone.h

#ifndef _MY_ZONE_H
#define _MY_ZONE_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include <stdlib.h>
#include <string.h>

#include "cstring.h"
#include "fileio.h"
#include "list.h"
#include "defaults.h"
#include "apply.h"
#include "extra.h"
#include "dir.h"
#include "area.h"
#include "help.h"
#include "mobile.h"
#include "object.h"
#include "room.h"
#include "reset.h"
#include "shop.h"
#include "special.h"

class Zone
{
   public:
      		Zone		();
      		~Zone		();
      void	NoChange	(int);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
      void	BaseVnum	(int);
      void	SetFileName	(char*);
      void	ImmLevel	(int, int);
      void	BaseLevel	(int);
      ifstream&	CountObjects	(ifstream&);
   private:
      Area	*_area;
      Help	*_help;
      Mobile	*_mobile;
      Object	*_object;
      Room	*_room;
      Reset	*_reset;
      Shop	*_shop;
      Special	*_special;
      List	_mobsrc;
      List	_mobdest;
      List	_objsrc;
      List	_objdest;
      List	_roomsrc;
      List	_roomdest;
};
ifstream& operator>>(ifstream&, Zone&);
ofstream& operator<<(ofstream&, Zone&);

#endif

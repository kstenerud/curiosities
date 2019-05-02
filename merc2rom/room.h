// room.h

#ifndef _MY_ROOM_H
#define _MY_ROOM_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "flags.h"
#include "extra.h"
#include "dir.h"
#include "list.h"
#include "defaults.h"

class Room
{
   public:
      		Room		();
		~Room		();
      void	BaseVnum	(int, int=-1, int=-1);
      int	FindLowVnum	(int=-1);
      int	FindHighVnum	(int=-1);
      Room*	FindVnum	(int);
      void	ListAdd		(List&);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      int	_vnum;
      Cstring	_name;
      Cstring	_desc;
      int	_area_num;
      Cstring	_room_flags;
      int	_sector;
      Dir	_dir[6];
      Extra	*_extras;
      int	_mana_recover;
      int	_hp_recover;
      Cstring	_clan_name;
      Room	*_next;
};
ifstream& operator>>(ifstream&, Room&);
ofstream& operator<<(ofstream&, Room&);

#endif

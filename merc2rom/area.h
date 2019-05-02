// area.h

#ifndef _MY_AREA_H
#define _MY_AREA_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "defaults.h"

class Area
{
   public:
      		Area		();
      		~Area		();
      void	SetFileName	(char*);
      void	SetLowRoom	(int);
      void	SetHighRoom	(int);
      void	SetLowLev	(int);
      void	SetHighLev	(int);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      Cstring	_filename;
      Cstring	_name;
      Cstring	_creator;
      Cstring	_levels;
      int	_lowroom;
      int	_highroom;
      int	_lowlev;
      int	_highlev;
};
ifstream& operator>>(ifstream&, Area&);
ofstream& operator<<(ofstream&, Area&);

#endif

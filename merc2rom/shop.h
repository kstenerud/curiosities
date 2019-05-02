// shop.h

#ifndef _MY_SHOP_H
#define _MY_SHOP_H

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include "cstring.h"
#include "fileio.h"
#include "defaults.h"

class Shop
{
   public:
      		Shop		();
      		~Shop		();
      void	BaseVnum	(int, int, int);
      ifstream&	Read		(ifstream&);
      ofstream&	Write		(ofstream&);
   private:
      int	_mobile;
      int	_types[5];
      int	_profit_buy;
      int	_profit_sell;
      int	_open;
      int	_close;
      Cstring	_comment;
      Shop	*_next;
};
ifstream& operator>>(ifstream&, Shop&);
ofstream& operator<<(ofstream&, Shop&);

#endif

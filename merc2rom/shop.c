// shop.cpp

#include "shop.h"

Shop::Shop()
{
   _mobile	= 0;
   for(int i = 0;i < 5;i++)
      _types[i]	= 0;
   _profit_buy	= 0;
   _profit_sell	= 0;
   _open	= 0;
   _close	= 0;
   _comment	= "";
   _next	= NULL;
}

Shop::~Shop()
{
}

void Shop::BaseVnum(int base, int low, int high)
{
   if(this == NULL)
      return;
   if(_mobile >= low && _mobile <= high)
      _mobile += base - low;
   _next->BaseVnum(base, low, high);
}

ifstream& Shop::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   _mobile		= ::read_number(stream);
   for(int i = 0;i<5;i++)
      _types[i]		= ::read_number(stream);
   _profit_buy		= ::read_number(stream);
   _profit_sell		= ::read_number(stream);
   _open		= ::read_number(stream);
   _close		= ::read_number(stream);
   if(!::is_eol(stream))
      _comment		= ::read_line(stream);

   if(!::is_next_word(stream,"0"))
   {
      _next = new Shop;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Shop::Write(ofstream& stream)
{
   if(this == NULL)
   {
      stream << "0" << endl << endl << endl;
      return stream;
   }

   stream << setw(6) << _mobile << setw(5) << _types[0] << setw(3)
          << _types[1] << setw(3) << _types[2] << setw(3)
          << _types[3] << setw(3) << _types[4] << setw(6)
          << _profit_buy << setw(4) << _profit_sell << setw(5)
          << _open << setw(3) << _close << "    ";
   stream << _comment << endl;

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Shop& shop)
{
   return shop.Read(stream);
}

ofstream& operator<<(ofstream& stream, Shop& shop)
{
   return shop.Write(stream);
}


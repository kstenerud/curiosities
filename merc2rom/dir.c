// dir.cpp

#include "dir.h"

Dir::Dir()
{
   _direction	= -1;
   _desc	= "";
   _name	= "";
   _state	= 0;
   _key		= 0;
   _dest_room	= 0;
}

Dir::~Dir()
{
}

void Dir::BaseVnum(int base, int low, int high)
{
   if(this == NULL)
      return;
   if(_dest_room >= low && _dest_room <= high)
      _dest_room += base - low;
}

ifstream& Dir::Read(ifstream& stream)
{
   if(this != NULL)
   {
      _direction	= ::read_num_pos(stream);
      _desc		= ::read_tilde(stream);
      _name		= ::read_tilde(stream);
      _state		= ::read_number(stream);
      _key		= ::read_number(stream);
      _dest_room	= ::read_number(stream);
   }
   return stream;
}

ofstream& Dir::Write(ofstream& stream)
{
   if(this != NULL && _direction != -1)
   {
      stream << "D" << _direction << endl;
      stream << _desc << endl;
      stream << _name << endl;
      stream << _state << " " << _key << " "
             << _dest_room << endl;
   }
   return stream;
}

ifstream& operator>>(ifstream& stream, Dir& dir)
{
   return dir.Read(stream);
}

ofstream& operator<<(ofstream& stream, Dir& dir)
{
   return dir.Write(stream);
}


// room.cpp

#include "room.h"

Room::Room()
{
   _vnum		= 0;
   _name		= "";
   _desc		= "";
   _area_num		= 0;
   _room_flags		= 0;
   _sector		= 0;
   _extras		= NULL;
   _mana_recover	= 100;
   _hp_recover		= 100;
   _clan_name		= "";
   _next		= NULL;
}

Room::~Room()
{
}

void Room::BaseVnum(int base, int low, int high)
{
   if(this == NULL)
      return;

   if(low == -1 || high == -1)
   {
      low = FindLowVnum();
      high = FindHighVnum();
   }

   for(int i=0;i<6;i++)
      _dir[i].BaseVnum(base, low, high);

   if(_vnum >= low && _vnum <= high)
      _vnum += base - low;
   _next->BaseVnum(base, low, high);
}

int Room::FindLowVnum(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _vnum < value)
      return _next->FindLowVnum(_vnum);
   else
      return _next->FindLowVnum(value);
}

int Room::FindHighVnum(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _vnum > value)
      return _next->FindHighVnum(_vnum);
   else
      return _next->FindHighVnum(value);
}

Room* Room::FindVnum(int value)
{
   if(this == NULL)
      return NULL;
   if(_vnum == value)
      return this;
   return _next->FindVnum(value);
}

void Room::ListAdd(List& list)
{
   if(this == NULL)
      return;
   list += _vnum;
   _next->ListAdd(list);
}

ifstream& Room::Read(ifstream& stream)
{
   int temp;

   if(this == NULL)
      return stream;

   while(!::is_next_word(stream, "#"))	// kill useless stuff
      ::read_line(stream);		// like mobprogs etc
   ::read_char(stream);			// get off the "#"
   _vnum		= ::read_number(stream);
   _name		= ::read_tilde(stream);
   _desc		= ::read_tilde(stream);
   			  ::read_number(stream);	// area number
   _room_flags		= ::make_flags(::read_number(stream));
   _sector		= ::read_number(stream);
   while(::check_next_char(stream) == 'D')
   {
      ::read_char(stream);				// "D"
      temp = ::check_next_char(stream) - '0';
      stream >> _dir[temp];		// read a direction
   }
   if(::is_next_word(stream, "E"))
   {
      _extras = new Extra;
      _extras->Read(stream);
   }
   			  ::read_word(stream);	// "S"

   if(!::is_next_word(stream, "#0"))
   {
      _next = new Room;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Room::Write(ofstream& stream)
{
   if(this == NULL)
   {
      stream << "#0" << endl << endl << endl;
      return stream;
   }

   stream << "#" << _vnum << endl;
   stream << _name << endl;
   stream << _desc << endl;
   stream << _area_num << " " << _room_flags << " " << _sector << endl;
   stream << _dir[0] << _dir[1] << _dir[2] << _dir[3] << _dir[4] << _dir[5];
   stream << *_extras;
   stream << "S" << endl;

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Room& room)
{
   return room.Read(stream);
}

ofstream& operator<<(ofstream& stream, Room& room)
{
   return room.Write(stream);
}


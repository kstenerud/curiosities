// zone.cpp

#include "zone.h"

Zone::Zone()
{
   _area	= NULL;
   _help	= NULL;
   _mobile	= NULL;
   _object	= NULL;
   _room	= NULL;
   _reset	= NULL;
   _shop	= NULL;
   _special	= NULL;
   _mobsrc	= "mobile";
   _mobdest	= "mobile";
   _objsrc	= "object";
   _objdest	= "object";
   _roomsrc	= "room";
   _roomdest	= "room";
}

Zone::~Zone()
{
   if(_area != NULL) delete _area;
   if(_mobile != NULL) delete _mobile;
   if(_object != NULL) delete _object;
   if(_room != NULL) delete _room;
   if(_reset != NULL) delete _reset;
   if(_shop != NULL) delete _shop;
   if(_special != NULL) delete _special;
}

void Zone::NoChange(int change)
{
   if(this == FALSE || !change)
      return;
   _mobile->NoChange();
   _object->NoChange();
}

void Zone::BaseVnum(int base)
{
   int lomob = _mobile->FindLowVnum();
   int himob = _mobile->FindHighVnum();
   int loobj = _object->FindLowVnum();
   int hiobj = _object->FindHighVnum();
   int loroom = _room->FindLowVnum();
   int hiroom = _room->FindHighVnum();

   if(this == NULL)
      return;

   if(base >= 0)
   {
      _reset->BaseVnum(base, lomob, himob, loobj, hiobj, loroom, hiroom);
      _shop->BaseVnum(base, lomob, himob);
      _special->BaseVnum(base, lomob, himob);
      _mobile->BaseVnum(base, lomob, himob);
      _object->BaseVnum(base, loobj, hiobj);
      _room->BaseVnum(base, loroom, hiroom);
   }

   _area->SetLowRoom(_room->FindLowVnum());
   _area->SetHighRoom(_room->FindHighVnum());
}

void Zone::SetFileName(char *in)
{
   _area->SetFileName(in);
}

void Zone::ImmLevel(int from, int to)
{
   if(from > 0)
      _help->ImmLevel(from, to);
}

void Zone::BaseLevel (int base)
{
   if(this == NULL)
      return;

   if(base > 0)
      _mobile->BaseLevel(base);
   else _mobile->BaseLevel(_mobile->FindLowLevel());
   _object->BaseLevel(_mobile->FindLowLevel());
   _reset->SetObjLevels(_mobile, _object);

   _area->SetLowLev(_mobile->FindLowLevel());
   _area->SetHighLev(_mobile->FindHighLevel());
}

ifstream& Zone::CountObjects(ifstream& stream)
{
   int		num;
   Cstring	str;

   if(this == NULL)
      return stream;

   do
   {
      str = read_word(stream);

      if(str == "#MOBILES")
      {
         skip_until(stream, '#');
         while( (num=read_number(stream)) != 0)
         {
            _mobsrc += num;
            skip_until(stream, '#');
         }
      }
      if(str == "#OBJECTS")
      {
         skip_until(stream, '#');
         while( (num=read_number(stream)) != 0)
         {
            _objsrc += num;
            skip_until(stream, '#');
         }
      }
      if(str == "#ROOMS")
      {
         skip_until(stream, '#');
         while( (num=read_number(stream)) != 0)
         {
            _roomsrc += num;
            skip_until(stream, '#');
         }
      }
   } while(str != "#$");

   _mobile->ListAdd(_mobdest);
   _object->ListAdd(_objdest);
   _room->ListAdd(_roomdest);
   _mobsrc.cmp(_mobdest);
   _objsrc.cmp(_objdest);
   _roomsrc.cmp(_roomdest);

   return stream;
}

ifstream& Zone::Read (ifstream& stream)
{
   Cstring str;

   if(this == NULL)
      return stream;

   do
   {
      str = read_word(stream);

      if(str == "#AREA")
      {
         _area = new Area;	// ******** need area filename
         stream >> *_area;
      }
      else if(str == "#HELPS" && !is_next_word(stream, "0 $~"))
      {
         _help = new Help;
         stream >> *_help;
      }
      else if(str == "#MOBILES" && !is_next_word(stream, "#0"))
      {
         _mobile = new Mobile;
         stream >> *_mobile;
      }
      else if(str == "#OBJECTS" && !is_next_word(stream, "#0"))
      {
         _object = new Object;
         stream >> *_object;
      }
      else if(str == "#ROOMS" && !is_next_word(stream, "#0"))
      {
         _room = new Room;
         stream >> *_room;
      }
      else if(str == "#RESETS" && !is_next_word(stream, "S"))
      {
         _reset = new Reset;
         stream >> *_reset;
      }
      else if(str == "#SHOPS" && !is_next_word(stream, "0"))
      {
         _shop = new Shop;
         stream >> *_shop;
      }
      else if(str == "#SPECIALS" && !is_next_word(stream, "S"))
      {
         _special = new Special;
         stream >> *_special;
      }
   } while(str != "#$");
   return stream;
}

ofstream& Zone::Write (ofstream& stream)
{
   if(this == NULL)
      return stream;

   if(_area != NULL)
   {
      stream << "#AREA" << endl;
      stream << *_area;
   }
   stream << "#HELPS" << endl;
   stream << *_help;
   stream << "#MOBILES" << endl;
   stream << *_mobile;
   stream << "#OBJECTS" << endl;
   stream << *_object;
   stream << "#ROOMS" << endl;
   stream << *_room;
   stream << "#RESETS" << endl;
   stream << *_reset;
   stream << "#SHOPS" << endl;
   stream << *_shop;
   stream << "#SPECIALS" << endl;
   stream << *_special;
   stream << "#$" << endl;

   return stream;
}

ifstream& operator>>(ifstream& stream, Zone& zone)
{
   return zone.Read(stream);
}

ofstream& operator<<(ofstream& stream, Zone& zone)
{
   return zone.Write(stream);
}

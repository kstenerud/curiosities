// object.cpp

#include "object.h"

const char *dmg_table[] =
{	//13
	"hit",
	"slice",
	"stab",
	"slash",
	"whip",
	"claw",
	"blast",
	"pound",
	"crush",
	"grep",
	"bite",
	"pierce",
	"suction"
};

const char *weapon_table [] =
{	//13
	"staff",
	"sword",
	"dagger",
	"sword",
	"whip",
	"exotic",
	"exotic",
	"club",
	"mace",
	"exotic",
	"exotic",
	"dagger",
	"exotic"
};

const char *item_table [] =
{	//27
	"",
	"light",
	"scroll",
	"wand",
	"staff",
	"weapon",
	"",
	"",
	"treasure",
	"armor",
	"potion",
	"",
	"furniture",
	"trash",
	"",
	"container",
	"",
	"drink",
	"key",
	"food",
	"money",
	"",
	"boat",
	"npc_corpse",
	"pc_corpse",
	"fountain",
	"pill"
};

const char *liquid_table [] =
{	//16
	"'water'",
	"'beer'",
	"'wine'",
	"'ale'",
	"'dark ale'",
	"'whisky'",
	"'lemonade'",
	"'firebreather'",
	"'local specialty'",
	"'slime mold juice'",
	"'milk'",
	"'tea'",
	"'coffee'",
	"'blood'",
	"'salt water'",
	"'cola'"
};


const char *spell_table [] =
{	//207
	"''",			//   0
	"'armor'",
	"'teleport'",
	"'bless'",
	"'blindness'",
	"'burning hands'",
	"'call lightning'",
	"'charm person'",
	"'chill touch'",
	"''",
	"'colour spray'",	//  10
	"'control weather'",
	"'create food'",
	"'create water'",
	"'cure blindness'",
	"'cure critical'",
	"'cure light'",
	"'curse'",
	"'detect evil'",
	"'detect invis'",
	"'detect magic'",	//  20
	"'detect poison'",
	"'dispel evil'",
	"'earthquake'",
	"'enchant weapon'",
	"'energy drain'",
	"'fireball'",
	"'harm'",
	"'heal'",
	"'invis'",
	"'lightning bolt'",	//  30
	"'locate object'",
	"'magic missile'",
	"'poison'",
	"'protection evil'",
	"'remove curse'",
	"'sanctuary'",
	"'shocking grasp'",
	"'sleep'",
	"'giant strength'",
	"'summon'",		//  40
	"'ventriloquate'",
	"'word of recall'",
	"'cure poison'",
	"'detect hidden'",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			//  50
	"''",
	"''",
	"'identify'",
	"''",
	"''",
	"'fly'",
	"'continual light'",
	"'know alignment'",
	"'dispel magic'",
	"''",			//  60
	"'cure serious'",
	"'cause light'",
	"'cause critical'",
	"'cause serious'",
	"'flamestrike'",
	"'stone skin'",
	"'shield'",
	"'weaken'",
	"'mass invis'",
	"'acid blast'",		//  70
	"''",
	"'faerie fire'",
	"'faerie fog'",
	"'pass door'",
	"''",
	"''",
	"'infravision'",
	"''",
	"''",
	"'create spring'",	//  80
	"'refresh'",
	"'change sex'",
	"'gate'",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			//  90
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 100
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 110
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 120
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 130
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 140
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 150
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 160
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 170
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 180
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",			// 190
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"''",
	"'acid breath'",	// 200
	"'fire breath'",
	"'frost breath'",
	"'gas breath'",
	"'lightning breath'",
	"'general purpose'",
	"'high explosive'"	// 206
};

Object::Object()
{
   _vnum	= 0;
   _name	= "";
   _short_desc	= "";
   _long_desc	= "";
   _action	= "";
   _material	= DEF_OBJ_MATERIAL;
   _type	= "";
   _extra_flags	= 0;
   _wear_flags	= 0;
   _v0		= 0;
   _v1		= 0;
   _v2		= 0;
   _v3		= 0;
   _v4		= 0;
   _level	= 1;
   _cost	= 0;
   _applies	= NULL;
   _extras	= NULL;
   _nochange	= FALSE;
   _next	= NULL;
}

void Object::NoChange()
{
   if(this == NULL)
      return;

   _nochange = TRUE;
   _next->NoChange();  
}

void Object::BaseVnum(int base, int low, int high)
{
   if(this == NULL)
      return;

   if(low == -1 || high == -1)
   {
      low = FindLowVnum();
      high = FindHighVnum();
   }

   if(_vnum >= low && _vnum <= high)
      _vnum += base - low;
   _next->BaseVnum(base, low, high);
}

int Object::FindLowVnum(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _vnum < value)
      return _next->FindLowVnum(_vnum);
   else
      return _next->FindLowVnum(value);
}

int Object::FindHighVnum(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _vnum > value)
      return _next->FindHighVnum(_vnum);
   else
      return _next->FindHighVnum(value);
}

Object* Object::FindVnum(int value)
{
   if(this == NULL)
      return NULL;
   if(_vnum == value)
      return this;
   return _next->FindVnum(value);
}

void Object::BaseLevel(int base)
{
   if(this == NULL)
      return;

   SetLevel(base);
   _next->BaseLevel(base);
}

void Object::SetLevel(int value)
{
   if(this == NULL)
      return;

   _level = value;

   if(_nochange)
      return;

   _cost = 100 * _level;
   if(_type == "weapon")
   {
      _v1 = (_level / 10) > 1 ? (_level / 10) : 1;			// #dice
      _v2 = (_level + 3) / ((_level/10) > 1 ? (_level/10) : 1);	// dice sides
   }
   else if(_type == "scroll" || _type == "potion" || _type == "pill"
        || _type == "wand" || _type == "staff")
      _v0 = _level;
   else if(_type == "armor")
   {
      int temp = (_level / 4) > 1 ? (_level / 4) : 1;
      _v0 = _v1 = _v2 = temp; 
      _v3 = (temp-5) > 1 ? (temp-5) : 1;
   }
}

void Object::ListAdd(List& list)
{
   if(this == NULL)
      return;
   list += _vnum;
   _next->ListAdd(list);
}

ifstream& Object::Read(ifstream& stream)
{
   int temp_type = 0;
   int trash = 0;

   if(this == NULL)
      return stream;

   while(!::is_next_word(stream, "#"))	// kill useless stuff
      ::read_line(stream);		// like mobprogs etc
   ::read_char(stream);		// get off the "#"
   _vnum		= ::read_number(stream);
   _name		= ::read_tilde(stream);
   _short_desc		= ::read_tilde(stream);
   _long_desc		= ::read_tilde(stream);
   _action		= ::read_tilde(stream);
   _type		= item_table[temp_type = ::read_num_pos(stream)];
   _extra_flags		= ::make_flags(::read_number(stream));
   _wear_flags		= ::make_flags(::read_number(stream));
   switch(temp_type)
   {
   case 1:	// light
      			  ::read_number(stream);
			  ::read_number(stream);
      _v0		= 0;
      _v1		= 0;
      _v2		= ::read_number(stream);	// hours remaining
			  ::read_number(stream);
      _v3		= 0;
      _v4		= 0;
      break;
   case 2:	// scroll
   case 10:	// potion
   case 26:	// pill
      _v0		= ::read_number(stream);	// level
      _v1		= spell_table[::read_num_pos(stream)];	// spell 1
      _v2		= spell_table[::read_num_pos(stream)];	// spell 2
      _v3		= spell_table[::read_num_pos(stream)];	// spell 3
      _v4		= 0;
      break;
   case 3:	// wand
   case 4:	// staff
      _v0		= ::read_number(stream);	// level
      _v1		= ::read_number(stream);	// max charges
      _v2		= ::read_number(stream);	// current charges
      _v3		= spell_table[::read_num_pos(stream)]; // spell
      _v4		= 0;
      break;
   case 5:	// weapon
      _v1		= ::read_number(stream); //*** # dice (may not be set)
      _v2		= ::read_number(stream); //*** dice sides (may not set)
      			  ::read_number(stream);
      trash		= ::read_num_pos(stream);
      _v0		= weapon_table[trash];	// weapon type
      _v3		= dmg_table[trash];	// damage type
      _v4		= 0;
      break;
   case 8:	// treasure
   case 13:	// trash
   case 18:	// key
   case 22:	// boat
   case 23:	// npc_corpse
   case 24:	// pc_corpse
   default:
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
      _v0		= 0;
      _v1		= 0;
      _v2		= 0;
      _v3		= 0;
      _v4		= 0;
      break;
   case 9:	// armor
      trash		= ::read_number(stream); //*** ac (may not be set)
      			  ::read_number(stream);
      			  ::read_number(stream);
      			  ::read_number(stream);
      _v0		= trash;		// pierce
      _v1		= trash;		// bash
      _v2		= trash;		// slash
      _v3		= (trash-5) > 1 ? (trash-5) : 1;	// exotic
      _v4		= 1;			//*** bulk
      break;
   case 12:	// furniture
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
      _v0		= 10;			// max people
      _v1		= 10000;		// max weight
      _v2		= "E";			// sit on
      _v3		= 0;
      _v4		= 0;
      break;
   case 15:	// container
      _v0		= ::read_number(stream);	// capacity
      _v1		= ::make_flags(::read_number(stream)); // flags
      _v2		= ::read_number(stream);	// key
			  ::read_number(stream);
      _v3		= _v0;				// max object weight
      _v4		= 100;				// weight %
      break;
   case 17:	// drink
      _v0		= ::read_number(stream);	// capacity
      _v1		= ::read_number(stream);	// current quantity
      _v2		= liquid_table[::read_num_pos(stream)]; // liquid type
      _v3		= ::read_number(stream); // poison
      _v4		= 0;
      break;
   case 19:	// food
      _v0		= ::read_number(stream);	// full
      _v1		= _v0;			// hunger
      _v2		= 0;
			  ::read_number(stream);
			  ::read_number(stream);
      _v3		= ::read_number(stream); // poison
      _v4		= 0;
      break;
   case 20:	// money
      trash		= ::read_number(stream);
      _v0		= trash % 1000;		// silver
      _v1		= trash / 1000;		// gold
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
      _v2		= 0;
      _v3		= 0;
      _v4		= 0;
      break;
   case 25:	// fountain
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
			  ::read_number(stream);
      _v0		= 10000;		// capacity (not)
      _v1		= 10000;		// full (not)
      _v2		= liquid_table[0];	// liquid type
      _v3		= 0;
      _v4		= 0;
      break;
   }

   _weight		= ::read_number(stream);
   _cost		= ::read_number(stream);
   			  ::read_number(stream);	// cost per day

   while( ::is_next_word(stream, "A") || ::is_next_word(stream, "E") )
   {
      if(::is_next_word(stream, "A"))
      {
         if(_applies == NULL)
            _applies = new Apply;
         _applies->Read(stream);
      }
      if(::is_next_word(stream, "E"))
      {
         if(_extras == NULL)
            _extras = new Extra;
         _extras->Read(stream);
      }
   }

   if(!::is_next_word(stream, "#0"))
   {
      _next = new Object;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Object::Write(ofstream& stream)
{
   if(this == NULL)
   {
      stream << "#0" << endl << endl << endl;
      return stream;
   }

   stream << "#" << _vnum << endl;
   stream << _name << endl;
   stream << _short_desc << endl;
   stream << _long_desc << endl;
   stream << _material << endl;
   stream << _type << " " << _extra_flags << " " << _wear_flags << endl;
   stream << _v0 << " " << _v1 << " " << _v2 << " " << _v3 << " " << _v4
          << endl;
   stream << _level << " " << _weight << " " << _cost << " P" << endl;
   stream << *_applies;
   stream << *_extras;

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Object& object)
{
   return object.Read(stream);
}

ofstream& operator<<(ofstream& stream, Object& object)
{
   return object.Write(stream);
}

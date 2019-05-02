// mobile.cpp

#include "mobile.h"

const char *sex_table [] =
{	//3
	"none",
	"male",
	"female",
        "none"
};


const char *pos_table [] =
{	//8
	"dead",
	"mort",
	"incap",
	"stun",
	"sleep",
	"rest",
	"sit",
	"fight",
	"stand"
};

Mobile::Mobile()
{
   _vnum		= 0;
   _name		= "";
   _short_desc		= "";
   _long_desc		= "";
   _look		= "";
   _race		= DEF_MOB_RACE;
   _act_flags		= 0;
   _aff_flags		= 0;
   _align		= 0;
   _group		= DEF_MOB_GROUP;
   _level		= 0;
   _hit			= 0;
   _hpnodice		= 0;
   _hpsizedice		= 0;
   _hpplus		= 0;
   _mananodice		= DEF_MOB_MDICE;
   _manasizedice	= DEF_MOB_MSIZE;
   _manaplus		= DEF_MOB_MPLUS;
   _damnodice		= 0;
   _damsizedice		= 0;
   _damplus		= 0;
   _dam_type		= DEF_MOB_DAMTYPE;
   _pierce		= 0;
   _bash		= 0;
   _slash		= 0;
   _magic		= 0;
   _off_flags		= ::make_flags(DEF_MOB_OFF_FLAGS);
   _imm_flags		= ::make_flags(DEF_MOB_IMM_FLAGS);
   _res_flags		= ::make_flags(DEF_MOB_RES_FLAGS);
   _vuln_flags		= ::make_flags(DEF_MOB_VULN_FLAGS);
   _start_pos		= "stand";
   _def_pos		= "stand";
   _sex			= "male";
   _gold		= 0;
   _form		= ::make_flags(DEF_MOB_FORM);
   _parts		= ::make_flags(DEF_MOB_PARTS);
   _size		= DEF_MOB_SIZE;
   _material		= DEF_MOB_MATERIAL;
   _nochange		= FALSE;
   _next		= NULL;
}

Mobile::~Mobile()
{
}

void Mobile::NoChange()
{
   if(this == NULL)
      return;

   _nochange = TRUE;
   _next->NoChange();  
}

void Mobile::BaseVnum(int base, int low, int high)
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

int Mobile::FindLowVnum(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _vnum < value)
      return _next->FindLowVnum(_vnum);
   else
      return _next->FindLowVnum(value);
}

int Mobile::FindHighVnum(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _vnum > value)
      return _next->FindHighVnum(_vnum);
   else
      return _next->FindHighVnum(value);
}

Mobile* Mobile::FindVnum(int value)
{
   if(this == NULL)
      return NULL;
   if(_vnum == value)
      return this;
   return _next->FindVnum(value);
}

void Mobile::BaseLevel(int base, int low)
{
   if(this == NULL)
      return;

   if(low == -1)
      low = FindLowLevel();

   SetLevel(_level+base-low);
   _next->BaseLevel(base, low);
}

void Mobile::SetLevel(int value)
{
   int lo_hp;
   int hi_hp;
   int lo_dmg;
   int hi_dmg;

   if(this == NULL)
      return;

   _level = value;

   if(_nochange)
      return;

   if(_level < 13)
   {
      lo_hp = 12 * _level;
      hi_hp = 12 * (_level+1);
   }
   else
   {
      lo_hp = _level * _level;
      hi_hp = (_level+1) * (_level+1);
   }
   lo_dmg = _level / 3;
   hi_dmg = _level + 3;

   _hpnodice = 1;
   _hpsizedice = hi_hp - lo_hp;
   _hpplus = lo_hp - 1;

   _damnodice = 1;
   _damsizedice = hi_dmg - lo_dmg;
   _damplus = lo_dmg - 1;

   _pierce = _bash = _slash = 10 - ((_level * 2) / 3);
   _magic = _pierce + 5;
   _hit = _level / 6;
   _gold = 10 * _level;
}

int Mobile::GetLevel()
{
   if(this == NULL)
      return -1;
   return _level;
}

int Mobile::FindLowLevel(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _level < value)
      return _next->FindLowLevel(_level);
   else
      return _next->FindLowLevel(value);
}

int Mobile::FindHighLevel(int value)
{
   if(this == NULL)
      return value;

   if(value == -1 || _level > value)
      return _next->FindHighLevel(_level);
   else
      return _next->FindHighLevel(value);
}


void Mobile::ListAdd(List& list)
{
   if(this == NULL)
      return;
   list += _vnum;
   _next->ListAdd(list);
}

ifstream& Mobile::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   while(!::is_next_word(stream, "#"))	// kill useless stuff
      ::read_line(stream);		// like mobprogs etc
   ::read_char(stream);			// "#"
   _vnum		= ::read_number(stream);
   _name		= ::read_tilde(stream);
   _short_desc		= ::read_tilde(stream);
   _long_desc		= ::read_tilde(stream);
   _look		= ::read_tilde(stream);
   _act_flags		= ::make_flags(::read_number(stream));
   _aff_flags		= ::make_flags(::read_number(stream));
   _align		= ::read_number(stream);
   			  ::read_word(stream);		// "S"
   _level		= ::read_number(stream);
   _hit			= ::read_number(stream);
   _pierce		= ::read_number(stream);
   _bash		= _pierce;
   _slash		= _pierce;
   _magic		= _pierce + 5;
   _hpnodice		= ::read_number(stream);
   			  ::read_char(stream);		// "d"
   _hpsizedice		= ::read_number(stream);
   			  ::read_char(stream);		// "+"
   _hpplus		= ::read_number(stream);
   _damnodice		= ::read_number(stream);
   			  ::read_char(stream);		// "d"
   _damsizedice		= ::read_number(stream);
   			  ::read_char(stream);		// "+"
   _damplus		= ::read_number(stream);
   _gold		= ::read_number(stream);
   			  ::read_word(stream);		// exp
   _start_pos		= pos_table[::read_num_pos(stream)];
   _def_pos		= pos_table[::read_num_pos(stream)];
   _sex			= sex_table[::read_num_pos(stream)];

   if(!::is_next_word(stream, "#0"))
   {
      _next = new Mobile;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Mobile::Write(ofstream& stream)
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
   stream << _look << endl;
   stream << _race << endl;
   stream << _act_flags << " " << _aff_flags << " " << _align << " " << _group
          << endl;
   stream << _level << " " << _hit << " "
          << _hpnodice   << "d" << _hpsizedice << "+" << _hpplus   << " "
          << _mananodice << "d" << _manasizedice << "+" << _manaplus << " "
          << _damnodice  << "d" << _damsizedice << "+" << _damplus  << " "
          << _dam_type << endl;
   stream << _pierce << " " << _bash << " " << _slash << " " << _magic << endl;
   stream << _off_flags << " " << _imm_flags << " " << _res_flags << " "
          << _vuln_flags << endl;
   stream << _start_pos << " " << _def_pos << " " << _sex << " " << _gold
          << endl;
   stream << _form << " " << _parts << " " << _size << " " << _material << endl;

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Mobile& mobile)
{
   return mobile.Read(stream);
}

ofstream& operator<<(ofstream& stream, Mobile& mobile)
{
   return mobile.Write(stream);
}

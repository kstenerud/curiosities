// reset.cpp

#include "reset.h"

Reset::Reset()
{
   _type	= 0;
   _a		= 0;
   _b		= 0;
   _c		= 0;
   _d		= 0;
   _comment	= "";
   _next	= NULL;
}

Reset::~Reset()
{
}

void Reset::BaseVnum(int base, int lomob, int himob, int loobj, int hiobj,
                     int loroom, int hiroom)
{
   if(this == NULL)
      return;

   switch(_type)
   {
   case 'M':
      if(_a >= lomob && _a <= himob)
         _a += base - lomob;
      if(_c >= loroom && _c <= hiroom)
         _c += base - loroom;
      break;
   case 'O':
      if(_a >= loobj && _a <= hiobj)
         _a += base - loobj;
      if(_b >= loroom && _b <= hiroom)
         _b += base - loroom;
      break;
   case 'P':
      if(_a >= loobj && _a <= hiobj)
         _a += base - loobj;
      if(_b >= loobj && _b <= hiobj)
         _b += base - loobj;
      break;
   case 'G':
   case 'E':
      if(_a >= loobj && _a <= hiobj)
         _a += base - loobj;
      break;
   case 'D':
   case 'R':
      if(_a >= loroom && _a <= hiroom)
         _a += base - loroom;
      break;
   }
   _next->BaseVnum(base, lomob, himob, loobj, hiobj, loroom, hiroom);
}

void Reset::SetObjLevels(Mobile *mob_base, Object *obj_base, int level)
{
   if(this == NULL || mob_base == NULL || obj_base == NULL)
      return;

   if(_type == 'M')
      level = mob_base->FindVnum(_a)->GetLevel();
   else if( (_type == 'G' || _type == 'E') && level > 0)
      obj_base->FindVnum(_a)->SetLevel(level);
   else
      level = -1;

   if(_next != NULL)
      _next->SetObjLevels(mob_base, obj_base, level);
}

ifstream& Reset::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   ::read_advance(stream);
   _type = ::read_char(stream);

   switch(_type)
   {
   case 'M':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// mob
      _b			= ::read_number(stream);	// glob limit
      _c			= ::read_number(stream);	// room
      _d			= _b;				// loc limit
      break;
   case 'O':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// object
      				  ::read_number(stream);	// "0"
      _b			= ::read_number(stream);	// room
      break;
   case 'P':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// object
      				  ::read_number(stream);	// "0"
      _b			= ::read_number(stream);	// container
      _c			= 1;				// copies
      break;
   case 'G':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// object
      				  ::read_number(stream);	// "0"
      break;
   case 'E':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// object
      				  ::read_number(stream);	// "0"
      _b			= ::read_number(stream);	// wear_loc
      break;
   case 'D':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// room
      _b			= ::read_number(stream);	// direction
      _c			= ::read_number(stream);	// flags
      break;
   case 'R':
      				  ::read_word(stream);		// "0"
      _a			= ::read_number(stream);	// room
      _b			= ::read_number(stream);	// direction??
      break;
   default:
      stream.unget();			// put back for comment read
   }
   if(!::is_eol(stream))
      _comment			= ::read_line(stream);		// comment

   if(!::is_next_word(stream,"S"))
   {
      _next = new Reset;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Reset::Write(ofstream& stream)
{
   if(this == NULL)
   {
      stream << "S" << endl << endl << endl;
      return stream;
   }
   switch(_type)
   {
   case 'M':
      stream << "M 0" << setw(6) << _a << setw(6) << _b << setw(6) << _c
             << setw(6) << _d << setw(6) << " " << _comment << endl;
      break;
   case 'O':
      stream << "O 0" << setw(6) << _a << "     0" << setw(6) << _b
             << setw(12) << " " << _comment << endl;
      break;
   case 'P':
      stream << "P 0" << setw(6) << _a << "     0" << setw(6) << _b
             << setw(6) << _c << setw(8) << " " << _comment << endl;
      break;
   case 'G':
      stream << "G 0" << setw(6) << _a << "     0" << setw(20) << " "
             << _comment << endl;
      break;
   case 'E':
      stream << "E 0" << setw(6) << _a << "     0" << setw(6) << _b
             << setw(14) << " " << _comment << endl;
      break;
   case 'D':
      stream << "D 0" << setw(6) << _a << setw(6) << _b << setw(6) << _c
             << setw(12) << " " << _comment << endl;
      break;
   case 'R':
      stream << "R 0" << setw(6) << _a << setw(6) << _b << setw(18) << " "
             << _comment << endl;
      break;
   default:
      stream << _comment << endl;
   }

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Reset& reset)
{
   return reset.Read(stream);
}

ofstream& operator<<(ofstream& stream, Reset& reset)
{
   return reset.Write(stream);
}

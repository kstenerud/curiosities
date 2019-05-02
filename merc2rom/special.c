// special.cpp

#include "special.h"

Special::Special()
{
   _type	= 0;
   _mobile	= 0;
   _special	= "";
   _comment	= "";
   _next	= NULL;
}

Special::~Special()
{
}

void Special::BaseVnum(int base, int low, int high)
{
   if(this == NULL)
      return;
   if(_mobile >= low && _mobile <= high)
      _mobile += base - low;
   _next->BaseVnum(base, low, high);
}

ifstream& Special::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   ::read_advance(stream);
   _type = ::read_char(stream);

   switch(_type)
   {
   case 'M':
      _mobile		= ::read_number(stream);
      _special		= ::read_word(stream);
      break;
   default:
      stream.unget();			// put back for comment read
   }
   if(!::is_eol(stream))
      _comment			= ::read_line(stream);

   if(!::is_next_word(stream,"S"))
   {
      _next = new Special;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Special::Write(ofstream& stream)
{
   if(this == NULL)
   {
      stream << "S" << endl << endl << endl;
      return stream;
   }

   switch(_type)
   {
      case 'M':
         stream << "M" << setw(6) << _mobile << " " << _special << "\t"
                << _comment << endl;
         break;
      default:
         stream << _comment << endl;
   }

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Special& special)
{
   return special.Read(stream);
}

ofstream& operator<<(ofstream& stream, Special& special)
{
   return special.Write(stream);
}

// apply.cpp

#include "apply.h"

Apply::Apply()
{
   _apply	= 0;
   _value	= 0;
   _next	= NULL;
}

Apply::~Apply()
{
   if(_next != NULL)
      delete _next;
}

ifstream& Apply::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   if(_apply != 0)		// to handle swapping between E and A
   {
      if(_next == NULL)
         _next = new Apply;
      return _next->Read(stream);
   }
      			   ::read_word(stream);	// get off the "A"
   _apply		 = ::read_number(stream);
   _value		 = ::read_number(stream);
   if(::is_next_word(stream, "A"))
   {
      _next = new Apply;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Apply::Write(ofstream& stream)
{
   if(this == NULL)
      return stream;

   if(_apply != 0)
      stream << "A" << endl << _apply << " " << _value << endl;

   return _next->Write(stream);

}

ifstream& operator>>(ifstream& stream, Apply& apply)
{
   return apply.Read(stream);
}

ofstream& operator<<(ofstream& stream, Apply& apply)
{
   return apply.Write(stream);
}

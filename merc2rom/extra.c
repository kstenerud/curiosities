// extra.cpp

#include "extra.h"

Extra::Extra()
{
   _name	= "";
   _desc	= "";
   _next	= NULL;
}

Extra::~Extra()
{
   if(_next != NULL)
      delete _next;
}

ifstream& Extra::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   if(_name != "")		// to handle swapping between E and A
   {
      if(_next == NULL)
         _next = new Extra;
      _next->Read(stream);
   }
   else
   {
      		::read_word(stream);	// get off the "E"
      _name	= ::read_tilde(stream);
      _desc	= ::read_tilde(stream);
      if(::is_next_word(stream, "E"))
      {
         _next = new Extra;
         return _next->Read(stream);
      }
   }
   return stream;
}

ofstream& Extra::Write(ofstream& stream)
{
   if(this == NULL)
      return stream;

   if(_name != "")
   {
      stream << "E" << endl;
      stream << _name << endl;
      stream << _desc << endl;
   }

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Extra& extra)
{
   return extra.Read(stream);
}

ofstream& operator<<(ofstream& stream, Extra& extra)
{
   return extra.Write(stream);
}

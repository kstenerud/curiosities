// help.cpp

#include "help.h"

Help::Help()
{
   _level	= 0;
   _name	= "";
   _desc	= "";
   _next	= NULL;
}

Help::~Help()
{
   if(_next != NULL)
      delete _next;
}

void Help::ImmLevel(int from, int to)
{
   if(this == NULL)
      return;

   if(_level >= from)
      _level += (to - from);

   _next->ImmLevel(from, to);
}

ifstream& Help::Read(ifstream& stream)
{
   if(this == NULL)
      return stream;

   _level	= ::read_number(stream);
   _name	= ::read_tilde(stream);
   _desc	= ::read_tilde(stream);

   if(!::is_next_word(stream, "0 $~"))
   {
      _next = new Help;
      return _next->Read(stream);
   }
   return stream;
}

ofstream& Help::Write(ofstream& stream)
{
   if(this == NULL)
   {
      stream << "0 $~" << endl << endl << endl;
      return stream;
   }

   stream << _level << " " << _name << endl;
   stream << _desc << endl;

   return _next->Write(stream);
}

ifstream& operator>>(ifstream& stream, Help& help)
{
   return help.Read(stream);
}

ofstream& operator<<(ofstream& stream, Help& help)
{
   return help.Write(stream);
}

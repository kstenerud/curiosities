// area.cpp

#include "area.h"

Area::Area()
{
   _filename	= "noname.are";
   _name	= "";
   _creator	= "";
   _lowlev	= 0;
   _highlev	= 0;
   _lowroom	= 0;
   _highroom	= 0;
}

Area::~Area()
{
}

void Area::SetFileName(char *in)
{
   _filename = in;
}

void Area::SetLowRoom(int value)
{
   if(this != NULL)
      _lowroom = value;
}

void Area::SetHighRoom(int value)
{
   if(this != NULL)
      _highroom = value;
}

void Area::SetLowLev(int value)
{
   if(this != NULL)
      _lowlev = value;
}

void Area::SetHighLev(int value)
{
   if(this != NULL)
      _highlev = value;
}

ifstream& Area::Read(ifstream& stream)
{
   if(this != NULL)
   {
      _levels	= ::read_bracket(stream);
      _creator	= ::read_word(stream);
      _name	= ::read_tilde(stream);
   }

   return stream;
}

ofstream& Area::Write(ofstream& stream)
{
   if(this != NULL)
   {
      stream << _filename << "~" << endl;
      stream << _name << endl;
      stream << "{" << setw(2) << _lowlev << " " << setw(2) << _highlev << "}"
             << setw(7) << _creator << " " << _name << endl;
      stream << _lowroom << " " << _highroom << endl << endl << endl;
   }

   return stream;
}

ifstream& operator>>(ifstream& stream, Area& area)
{
   return area.Read(stream);
}

ofstream& operator<<(ofstream& stream, Area& area)
{
   return area.Write(stream);
}

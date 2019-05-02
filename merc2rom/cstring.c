// cstring.cpp

// the code in here is very messy right now.  Mabye someday I will clean it
// up =)

#include <fstream.h>
#include <iostream.h>
#include <iomanip.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cstring.h"

Cstring::Cstring(char *c)
{
   _len = ::strlen(c);
   _str = new char[_len+1];
   ::strcpy(_str,c);
   assert(_len == (int)::strlen(_str));
}

Cstring::Cstring(const Cstring& other)
{
   _len = other._len;
   _str = new char[_len+1];
   ::strcpy(_str,other._str);
   assert(_len == (int)::strlen(_str));
}

Cstring::~Cstring()
{
   delete [] _str;
}

Cstring& Cstring::operator=(const Cstring& other)
{					// assignment overloader
   if(this != NULL && this != &other)	// don't kill ourselves
   {
      delete [] _str;			// out with the old
      _len = other._len;		// in with the new
      _str = new char[_len+1];
      ::strcpy(_str,other._str);
      assert(_len == (int)::strlen(_str));
   }
   return *this;
}

Cstring& Cstring::operator=(const char *in)
{
   if(this != NULL)
   {
      delete [] _str;		// out with the old
      _len = ::strlen(in);
      _str = new char[_len+1];
      ::strcpy(_str,in);
      assert(_len == (int)::strlen(_str));
   }

   return *this;
}

Cstring& Cstring::operator=(int value)
{
   int	i = 0;
   int	j = 0;
   int	neg = (value < 0);

//   if(this == NULL)
//      return NULL;

   delete [] _str;

   if(neg)
      value *= -1;

   if(value == 0)
   {
      _len = 1;
      _str = new char[2];
      _str[0] = '0';
      _str[1] = 0;
   }
   else
   {
      for(i=value, _len=0;i!=0;i/=10, _len++); // calculate length needed
      _len += neg;				// add 1 if it's negative
      _str = new char[_len+1];		// make new + null terminator
      j = _len;
      _str[j--] = 0;			// null terminate it
      for(i=value;j>=0+neg;j--, i/=10)	// put in value
         _str[j] = i % 10 + '0';
      if(neg)
         _str[0] = '-';
   }
   assert(_len == (int)::strlen(_str));
   return *this;
}

// let it die if the cstring is NULL.
int Cstring::operator-(const Cstring& other)	// diff
{

   return ::strcmp(_str, other._str);
}

int Cstring::operator-(const char *other)	// diff
{
   return ::strcmp(_str, other);
}

int Cstring::operator<(const Cstring& other)	// is less than
{
   return ::strcmp(_str, other._str) < 0;
}

int Cstring::operator<(const char *other)	// is less than
{
   return ::strcmp(_str, other) < 0;
}

int Cstring::operator>(const Cstring& other)	// is greater than
{
   return ::strcmp(_str, other._str) > 0;
}

int Cstring::operator>(const char *other)	// is greater than
{
   return ::strcmp(_str, other) > 0;
}

int Cstring::operator<=(const Cstring& other)	// less than or equal
{
   return ::strcmp(_str, other._str) <= 0;
}

int Cstring::operator<=(const char *other)	// less than or equal
{
   return ::strcmp(_str, other) <= 0;
}

int Cstring::operator>=(const Cstring& other)	// greater than or equal
{
   return ::strcmp(_str, other._str) >= 0;
}

int Cstring::operator>=(const char *other)	// greater than or equal
{
   return ::strcmp(_str, other) >= 0;
}

int Cstring::operator==(const Cstring& other)	// is equal
{
   return ::strcmp(_str, other._str) == 0;
}

int Cstring::operator==(const char *other)	// is equal
{
   return ::strcmp(_str, other) == 0;
}

int Cstring::operator!=(const Cstring& other)	// is not equal
{
   return ::strcmp(_str, other._str) != 0;
}

int Cstring::operator!=(const char *other)	// is not equal
{
   return ::strcmp(_str, other) != 0;
}

Cstring Cstring::operator+(const Cstring& other)
{
   char *newstr = new char[_len+other._len+1];

   if(this == NULL) return NULL;
   ::strcpy(newstr, _str);
   ::strcat(newstr, other._str);
   assert(_len + other._len == (int)::strlen(newstr));

   Cstring new_cstring(newstr);

   return new_cstring;
}

Cstring Cstring::operator+(const char *other)
{
   char *newstr = new char[_len + ::strlen(other) +1];

   if(this == NULL) return NULL;
   ::strcpy(newstr, _str);
   ::strcat(newstr, other);
   assert((int)::strlen(newstr) == _len + (int)::strlen(other));

   Cstring new_cstring(newstr);

   return new_cstring;
}

Cstring Cstring::operator+(char other)
{
   char *newstr = new char[_len+2];

   if(this == NULL) return NULL;
   ::strcpy(newstr, _str);
   newstr[_len] = other;
   newstr[_len+1] = 0;

   assert(_len+1 == (int)::strlen(newstr));

   Cstring new_cstring(newstr);

   return new_cstring;
}

Cstring& Cstring::operator+=(const Cstring& other)
{
   char *newstr = new char[_len+other._len+1];

//   if(this == NULL) return NULL;
   ::strcpy(newstr, _str);
   ::strcat(newstr, other._str);

   _len += other._len;
   delete _str;
   _str = newstr;
   assert(_len == (int)::strlen(newstr));

   return *this;
}

Cstring& Cstring::operator+=(const char *other)
{
   char *newstr = new char[_len + ::strlen(other) +1];

//   if(this == NULL) return NULL;
   ::strcpy(newstr, _str);
   ::strcat(newstr, other);

   _len += ::strlen(other);
   delete _str;
   _str = newstr;
   assert(_len == (int)::strlen(newstr));

   return *this;
}

Cstring& Cstring::operator+=(char other)
{
   char *newstr = new char[_len+2];

//   if(this == NULL) return NULL;
   ::strcpy(newstr, _str);
   newstr[_len] = other;
   newstr[_len+1] = 0;

   _len++;
   delete _str;
   _str = newstr;
   assert(_len == (int)::strlen(newstr));

   return *this;
}


Cstring & Cstring::operator--()
{
//   if(this == NULL) return NULL;
   _str[_len-1] = 0;
   _len = ::strlen(_str);

   return *this;
}

void Cstring::cut()
{
   if(this == NULL) return;
   _str[_len-1] = 0;
   _len = ::strlen(_str);
}

char Cstring::operator[](int i)
{
   if(this == NULL) return 0;
   if(i <= _len)
      return _str[i];
   return 0;
}

char * Cstring::get()				// access string
{
   if(this == NULL) return NULL;
   return _str;
}

int Cstring::len()				// access length
{
   if(this == NULL) return 0;
   return _len;
}


ofstream& operator<<(ofstream& stream, Cstring& string)	// iostream
{							// overloaders
   if(&string != NULL)
      stream << string._str;
   return stream;
}

ostream& operator<<(ostream& stream, Cstring& string)	// iostream
{							// overloaders
   if(&string != NULL)
     stream << string._str;
   return stream;
}

ifstream& operator>>(ifstream& stream, Cstring& string)
{
   char temp[100];

   if(&string != NULL)
   {
      stream >> temp;
      string = temp;
   }
   return stream;
}

istream& operator>>(istream& stream, Cstring& string)
{
   char temp[100];

   if(&string != NULL)
   {
      stream >> temp;
      string = temp;
   }
   return stream;
}

int Cstring::get_int()
{
   int	i = 0;
   int	number = 0;
   int	temp = 0;
   int  neg = 0;

   if(this == NULL) return 0;
   neg = (_str[0] == '-');
   if(neg)
      i++;

   do
   {
      for( temp=0 ; _str[i]<='9' && _str[i]>='0' && i < _len; i++ )
{
         temp = (temp * 10) + _str[i] - '0';
}
      number |= temp;
   } while(_str[i++] == '|' && i < _len);

   if(neg)		// check for negative
      number *= -1;
   return number;
}

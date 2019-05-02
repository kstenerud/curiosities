#include "textstr.h"


int Textstring::get_number()
{
   char num[1000] = "0";
   int i;
   int j=0;

   for(i=0;i<_len && _string[i] == ' ';i++);

   for(;i<_len && _string[i] >= '0' && _string[i] <= '9';i++)
      num[j++] = _string[i];

   if(j>0)
      num[j] = 0;
   return atoi(num);
}

int Textstring::has_nonalpha_chars()
{
   char ch;

   for(int i=0;i<_len;i++)
   {
      ch = _string[i];
      if(!(ch >= 'a' && ch <= 'z') &&
         !(ch >= 'A' && ch <= 'Z') &&
         !(ch == '\n' || ch == '\r') )
         return 1==1;
   }
   return 1==0;
}

void Textstring::_handle_overflow()
{
   _overflow = FALSE;
   if(_max_len > 0)
   {
      if(_len >= _max_len)
      {
         _overflow = TRUE;
         _len = _max_len-1;
      }
      // make sure we don't allocate too much memory.
      if(_avail_len > _max_len)
         _avail_len = _max_len;
   }
}

Textstring::Textstring(char* str, unsigned long int maxlen)
: _overflow(FALSE), _len(::strlen(str)), _max_len(maxlen)
{
   // I add some extra mem to the mem allocated by the string to speed up
   // += operations.
   _avail_len = _len+1+TEXTSTRING_ADVANCE;

   // make sure we don't overflow
   _handle_overflow();

   // now make the string.
   _string = new char[_avail_len];
   ::strncpy(_string, str, _len);
   _string[_len] = 0;
}

Textstring::Textstring(const Textstring& s)
: _len(s._len), _overflow(s._overflow), _max_len(s._max_len)
{
   _avail_len = _len+1+TEXTSTRING_ADVANCE;
   _string = new char[_avail_len];
   ::strcpy(_string, s._string);
}

const Textstring& Textstring::operator=(const Textstring& s)
{
   if(this != &s)
   {
      delete [] _string;
      _len = s._len;
      _avail_len = _len+1+TEXTSTRING_ADVANCE;
      _handle_overflow();
      _string = new char[_avail_len];
      ::strncpy(_string, s._string, _len);
      _string[_len] = 0;
   }
   return *this;
}

const Textstring& Textstring::operator=(char* str)
{
   delete [] _string;
   _len = ::strlen(str);
   _avail_len = _len+1+TEXTSTRING_ADVANCE;
   _handle_overflow();
   _string = new char[_avail_len];
   ::strncpy(_string, str, _len);
   _string[_len] = 0;

   return *this;
}

const Textstring& Textstring::operator=(char s)
{
   delete [] _string;
   _len = 1;
   _avail_len = _len+1+TEXTSTRING_ADVANCE;
   _handle_overflow();
   _string = new char[_avail_len];
   _string[0] = s;
   _string[1] = 0;

   return *this;
}

Textstring Textstring::operator+(const Textstring& s) const
{
   char* str = new char[_len + s._len + 1];

   ::strcpy(str, _string);
   ::strcat(str, s._string);

   return Textstring(str);
}

Textstring Textstring::operator+(char* s) const
{
   char* str = new char[_len + ::strlen(s) + 1];

   ::strcpy(str, _string);
   ::strcat(str, s);

   return Textstring(str);
}

Textstring Textstring::operator+(char s) const
{
   char* str = new char[_len + 2];

   ::strcpy(str, _string);
   str[_len] = s;
   str[_len+1] = 0;

   return Textstring(str);
}

const Textstring& Textstring::operator+=(const Textstring& s)
{
   char* str;
   int oldlen = _len;

   // calculate required length.
   _len += s._len;

   // if it'll fit, we just strcat it in.
   if(_len < _avail_len)
      ::strcat(_string, s._string);
   else
   {
      // otherwise we hav to delete and rebuild.
      _avail_len = _len + s._len + 1 + TEXTSTRING_ADVANCE;
      _handle_overflow();
      str = new char[_avail_len];

      ::strcpy(str, _string);
      ::strncat(str, s._string, (_len-oldlen));
      str[_len] = 0;

      delete [] _string;
      _string = str;
   }

   return *this;
}

const Textstring& Textstring::operator+=(char* s)
{
   char* str;
   int   length = strlen(s);
   int   oldlen = _len;

   _len += length;
   if(_len < _avail_len)
      ::strcat(_string, s);
   else
   {
      _avail_len = _len + length + 1 + TEXTSTRING_ADVANCE;
      _handle_overflow();
      str = new char[_avail_len];

      ::strcpy(str, _string);
      ::strncat(str, s, (_len-oldlen));
      str[_len] = 0;

      delete [] _string;
      _string = str;
   }

   return *this;
}

const Textstring& Textstring::operator+=(char s)
{
   char* str;

   _len++;
   if(_len < _avail_len)
   {
      _string[_len-1] = s;
      _string[_len] = 0;
   }
   else
   {
      _avail_len += 1+TEXTSTRING_ADVANCE;
      _handle_overflow();
      str = new char[_avail_len];

      ::strcpy(str, _string);
      if(_len < (_avail_len-1))
      {
         str[_len-1] = s;
         str[_len] = 0;
      }

      delete [] _string;
      _string = str;
   }

   return *this;
}

Textstring Textstring::extract_line()
{
   Textstring dest;
   int i = 0;

   for(;i<_len;i++)
   {
      // break on a newline.
      if(_string[i] == '\n')
      {
         i++;
         break;
      }
      // read any printable character except for tilde.  (tilde will be a
      // reserved value)
      if(_string[i] >= ' ' && _string[i] < '~')  // no tildes
         dest += _string[i];
   }

   // if we found a newline, erase up to that newline.
   if(i > 0 && _string[i-1] == '\n')
   {
      clear_overflow();
      ::strcpy( _string, (_string+i));
      _len = ::strlen(_string);
   }
   else dest = "";

   // now return the data we extracted.
   return dest;
}

Textstring Textstring::extract_argument()
{
   Textstring dest;
   int i = 0;

   // get off any whitespace.
   for(;i<_len;i++)
      if(_string[i] != ' ')
         break;

   // read until space or newline.
   // note that this function will not eat a newline, so if one is encountered,
   // it will keep sticking on the newline until this textstring is cleared.
   for(;i<_len;i++)
   {
      if(_string[i] == '\n' || _string[i] == ' ')
         break;
      if(_string[i] > ' ' && _string[i] < '~')  // no tildes or spaces
         dest += _string[i];
   }

   // skip over any trailing spaces.
   for(;i<_len;i++)
      if(_string[i] != ' ')
         break;

   // clear that part of the string if we found anything.
   if(i > 0)
   {
      clear_overflow();
      ::strcpy( _string, (_string+i));
      _len = ::strlen(_string);
   }
   return dest;
}

int Textstring::has_newline()
{
   int i = 0;

   for(;i<_len;i++)
      if(_string[i] == '\n')
         return TRUE;
   return FALSE;
}

#ifdef _TEXTSTR_USE_IOSTREAM
istream& operator>>(istream& in, Textstring& s)
{
   char* str = new char[1000];
   in >> str;
   s = str;
   return in;

}

ostream& operator<<(ostream& out, Textstring& s)
{
   out << s.get_string();
}
#endif

#ifndef _TEXTSTR_H
#define _TEXTSTR_H

#include <stdlib.h>

// This is my custom string implementation.  I'm a bit old fashioned in that
// I prefer not to derive if I can avoid it.

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1;
#endif

// I comment this out when I don't plan to use iostrems since there's a problem
// with the headers in redhat 4.0 distribution for DEC Alpha.
// #define _TEXTSTR_USE_IOSTREAM

#include <string.h>
#include <assert.h>

#define TEXTSTRING_ADVANCE 10

class Textstring
{
public:
                     Textstring(char* = "", unsigned long int = 0);
                     Textstring(const Textstring&);
                     ~Textstring()                          {delete [] _string;}
         int         operator==(const Textstring& s) const
                                   {return (::strcmp(_string, s._string) == 0);}
         int         operator!=(const Textstring& s) const
                                   {return (::strcmp(_string, s._string) != 0);}
         int         operator> (const Textstring& s) const
                                   {return (::strcmp(_string, s._string) >  0);}
         int         operator< (const Textstring& s) const
                                   {return (::strcmp(_string, s._string) <  0);}
         int         operator<=(const Textstring& s) const
                                   {return (::strcmp(_string, s._string) >= 0);}
         int         operator>=(const Textstring& s) const
                                   {return (::strcmp(_string, s._string) <= 0);}
         int         operator==(const char* s) const
                                           {return (::strcmp(_string, s) == 0);}
         int         operator!=(const char* s) const
                                           {return (::strcmp(_string, s) != 0);}
         int         operator> (const char* s) const
                                           {return (::strcmp(_string, s) >  0);}
         int         operator< (const char* s) const
                                           {return (::strcmp(_string, s) <  0);}
         int         operator<=(const char* s) const
                                           {return (::strcmp(_string, s) >= 0);}
         int         operator>=(const char* s) const
                                           {return (::strcmp(_string, s) <= 0);}

         char&       operator[](unsigned int pos)
                                      {assert(pos <= _len);return _string[pos];}
   const Textstring& operator= (const Textstring&);
   const Textstring& operator= (char*);
   const Textstring& operator= (char);
         Textstring  operator+ (const Textstring&) const;
         Textstring  operator+ (char*) const;
         Textstring  operator+ (char) const;
   const Textstring& operator+=(const Textstring&);
   const Textstring& operator+=(char* s);
   const Textstring& operator+=(char s);
   // get the char* string contained in this object.
         char*       get_string()                              {return _string;}
   // length of the string.
   unsigned long int get_length()                              {return _len;}
   // extract chars until a newline is encountered.
         Textstring  extract_line();
   // extract chars until a white space is encountered.
         Textstring  extract_argument();
   // check if there's a newline in here.
         int         has_newline();
   // check for overflow.
         int         is_overflow()                           {return _overflow;}
   // clear the overflow state.
         void        clear_overflow()                       {_overflow = FALSE;}
   // check if empty.
         int         is_empty()                              {return _len == 0;}
         int has_nonalpha_chars();
         int get_number();
private:
   // checks if there's been an overflow.
   void  _handle_overflow();
   char* _string;				// the actual string
   unsigned long int _len;			// length of the string
   unsigned long int   _avail_len;		// memory available
   unsigned long int   _max_len;		// max allowable length
   unsigned long int   _overflow;		// overflow flag
};

#ifdef _TEXTSTR_USE_IOSTREAM
#include <iostream.h>
istream& operator>>(istream&, Textstring&);
ostream& operator<<(ostream&, Textstring&);
#endif

#endif

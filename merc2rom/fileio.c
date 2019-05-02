// fileio.cpp

#include <string.h>
#include <stdlib.h>
#include "fileio.h"

// *************** File I/O stuff ***************


char read_char(ifstream& stream)
{
   char		value;

   stream.get(value);
   if(stream.eof())
   {
      cout << "ERROR: EOF Encountered" << endl;
      exit(5);
   }
   return value;
}

Cstring read_while(ifstream& stream, char from, char to)
{
   Cstring	temp;
   char		read;

   if(from <= to)
      while( (read = read_char(stream)) >= from && read <= to )
         temp += read;
   else
      while( (read = read_char(stream)) >= from || read <= to )
         temp += read;

   stream.unget();
   return temp;
}

void skip_while(ifstream& stream, char from, char to)
{
   char		read;
   if(from <= to)
      while( (read = read_char(stream)) >= from && read <= to );
   else
      while( (read = read_char(stream)) >= from || read <= to );

   stream.unget();
}


Cstring read_until(ifstream& stream, char to)
{
   Cstring	temp;
   char		read;

   while((read = read_char(stream)) != to)
      temp += read;

   return temp;
}

void skip_until(ifstream& stream, char to)
{
   while(read_char(stream) != to);
}

void read_advance(ifstream& stream)
{
   skip_while(stream, '~'+1, '!'-1);
}

char check_next_char(ifstream& stream)
{
   char		value;

   read_advance(stream);
   value = read_char(stream);
   stream.unget();
   return value;
}

Cstring check_next(ifstream& stream, int num)
{
   Cstring	string;
   int		i;

   read_advance(stream);

   for(i=0;i<num;i++)
      string += read_char(stream);

   for(i=0;i<num;i++)
      stream.unget();
   return string;
}

int is_next_word(ifstream& stream, char *in)
{
   read_advance(stream);
   return (check_next(stream, strlen(in)) == in);
}

int is_eol(ifstream& stream)
{
   int	i;
   char	value = read_char(stream);

   for(i=1;value != 10 && (value >= '~' || value <= '!');i++)
      value = read_char(stream);

   for(;i>0;i--)
      stream.unget();

   return (value == 10);
}

Cstring read_word(ifstream& stream)
{
   read_advance(stream);
   return read_while(stream, '!', '~');
}

Cstring read_line(ifstream& stream)
{
   read_advance(stream);
   return read_until(stream, 10);
}

Cstring read_tilde(ifstream& stream)
{
   read_advance(stream);
   return read_until(stream, '~') + '~';
}

Cstring read_bracket(ifstream& stream)
{
   read_advance(stream);
   return read_until(stream, '}') + '}';
}

int read_number(ifstream& stream)
{
   char read;
   int	number = 0;
   int	temp = 0;
   int  neg = 0;

   read_advance(stream);

   if(read_char(stream) == '-')
      neg = 1;
   else stream.unget();

   do
   {
      for( temp=0, read=read_char(stream)
       ;read<='9' && read>='0' ; read=read_char(stream) )
         temp = (temp * 10) + read - '0';
      number += temp;
   } while(read == '|');

   stream.unget();	// put back last char since it wasn't a numeral
   if(neg)		// check for negative
      number *= -1;
   return number;
}

int read_num_pos(ifstream& stream)
{
   int number = read_number(stream);
   if(number < 0)
      number = 0;
   return number;
}

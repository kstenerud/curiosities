// flags.cpp

#include "flags.h"

Cstring make_flags(int value)
{
   int		i = 1;
   int		j = 0;
   Cstring	string;

   if(value == 0)
      string = 0;
   else
   {
      for(j=0,i=1;j<26;j++,i=i<<1)
         if(value & i)
            string += 'A' + j;
      for(j=0;j<8;j++,i=i<<1)
         if(value & i)
            string += 'a' + j;
   }

   return string;
}

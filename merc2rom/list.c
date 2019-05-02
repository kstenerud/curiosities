// list.cpp

#include <iomanip.h>
#include "list.h"

List::List(char *instring)
{
   _str = instring;
   _array = new int[1];
   _len = 0;
}


List::~List()
{
   if(_array != NULL)
      delete [] _array;
}

void List::operator=(char* in)
{
   _str = in;
}

List& List::operator+=(int value)
{
   int	i;
   int	*temp;

   temp = new int[_len+1];

   if(_len != 0)
      for(i=0;i<_len;i++)
         temp[i] = _array[i];
   _len++;
   temp[_len-1] = value;

   delete [] _array;
   _array = temp;

   return *this;
}

int& List::operator[](int i)
{
   return _array[i];
}

ostream& operator<<(ostream& stream, List& list)	// iostream
{
   for(int i = 0;i<list._len;i++)
      stream << list._array[i] << endl;

   return stream;
}

void List::cmp(const List& other)
{
   int i;
   int j;

   for(i=0, j=0;i<_len;i++)
      if( (_array[i] != other._array[i-j]) )
      {
         cout << "Warning: lost " << _str << " " << _array[i]
                << " in conversion!" << endl;
         j++;
      }
}

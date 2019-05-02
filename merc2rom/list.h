// list.h

#ifndef _MY_LIST_H
#define _MY_LIST_H

#include "cstring.h"

class List
{
   friend ostream& operator<<(ostream& stream, List& list);
   public:
      List(char * = "nothing");
      ~List();
      List&	operator+=	(int value);
      void	operator=	(char*);
      int&	operator[]	(int i);
      void	cmp		(const List&);
   private:
      Cstring	_str;
      int	*_array;
      int	_len;
};
#endif

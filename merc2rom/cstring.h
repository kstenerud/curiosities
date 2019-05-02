// cstring.h

#ifndef _MY_CSTRING_H
#define _MY_CSTRING_H

#include <fstream.h>
#include <iostream.h>

class Cstring
{
	friend	ofstream& operator<<(ofstream&, Cstring&);
	friend	ostream& operator<<(ostream&, Cstring&);
	friend	ifstream& operator>>(ifstream&, Cstring&);
	friend	istream& operator>>(istream&, Cstring&);
    public:
 	Cstring(char * = "");	// convert char array into a Cstring
 	Cstring(const Cstring&);
 	~Cstring();
 	Cstring& operator=(const Cstring&);
	Cstring& operator=(const char *);
        Cstring& operator=(int);
 	int operator-(const Cstring&);		// diff
 	int operator-(const char *);		// diff
 	int operator<(const Cstring&);		// less than
 	int operator<(const char *);		// less than
 	int operator>(const Cstring&);		// greater than
 	int operator>(const char *);		// greater than
	int operator<=(const Cstring&);		// less than or equal
	int operator<=(const char *);		// less than or equal
  	int operator>=(const Cstring&);		// greater than or equal
  	int operator>=(const char *);		// greater than or equal
 	int operator==(const Cstring&);		// equal
 	int operator==(const char *);		// equal
	int operator!=(const Cstring&);		// not equal
	int operator!=(const char *);		// not equal
 	Cstring operator+(const Cstring&);	// string concat
 	Cstring operator+(const char *);	// string concat
 	Cstring operator+(const char);		// string concat
        Cstring& operator+=(const Cstring&);
        Cstring& operator+=(const char *);
        Cstring& operator+=(char);
	Cstring& operator--();
        void Cstring::cut();
        char operator[](int);
 	char * get();				// accessor method;
	int len();				// get string length;
        int get_int();				// get integer value, if any;
     private:
 	char *	_str;				// the string
	int 	_len;				// length of string
};
#endif

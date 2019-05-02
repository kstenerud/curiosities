// fileio.h

#ifndef _MY_FILEIO_H
#define _MY_FILEIO_H

#include <iostream.h>
#include <fstream.h>
#include "cstring.h"

//----------------------------------------------------------
// extract 1 character from an input filestream
// INPUT:  ifstream&  (input file stream)
// OUTPUT: char       (the next character in the stream)
char	read_char	(ifstream&);

//----------------------------------------------------------
// read characters from input stream while they are anywhere
// from the first char to the second char.  This supports
// wraparound (ie. from '~' to '!' reads while characters are
// non-printable
// INPUT:  ifstream&  (input file stream)
//         char       ("from" character)
//         char       ("to" character)
// OUTPUT: Cstring    (string of characters read)
Cstring	read_while	(ifstream&, char, char);

//----------------------------------------------------------
// extract characters from input filestream and discard them
// while they are anywhere from the first char to the second
// char.  This supports wraparound. (see read_while)
// INPUT:  ifstream&  (input file stream)
//         char       ("from" character)
//         char       ("to" character)
// OUTPUT: void
void	skip_while	(ifstream&, char, char);

//----------------------------------------------------------
// read characters from input filestream until char is reached.
// this function also extracts the char reached!
// INPUT:  ifstream&  (input file stream)
//         char       (char to read to)
// OUTPUT: Cstring    (string of characters read)
Cstring	read_until	(ifstream&, char);

//----------------------------------------------------------
// extract characters from input filestream and discard until
// char is reached.  This function will also discard the char
// reached.
// INPUT:  ifstream&  (input file stream)
//         char       (char to read to)
// OUTPUT: void
void	skip_until	(ifstream&, char);

//----------------------------------------------------------
// extract and discard all following non-printable characters
// in the input filestream until a printable character is found.
// INPUT:  ifstream&  (input file stream)
// OUTPUT: void
void	read_advance	(ifstream&);

//----------------------------------------------------------
// check the next character in an input filestream
// (character is not extracted from the stream)
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: char       (the next character in the stream)
char	check_next_char	(ifstream&);

//----------------------------------------------------------
// check the next x characters in an input stream
// (characters are not extracted from the stream)
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
//         int        (number of characters to check)
// OUTPUT: Cstring    (string of characters we checked)
Cstring	check_next	(ifstream&, int = 1);

//----------------------------------------------------------
// compare to next word in the input file stream
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
//         char*      (string to compare to)
// OUTPUT: int        (true or false)
int	is_next_word	(ifstream&, char *);

//----------------------------------------------------------
//  check if we are at an end-of-line (10).  This function will
// skip over non-printables.  No characters are extracted from
// the filestream.
// INPUT:  ifstream&  (input file stream)
// OUTPUT: int        (true or flase)
int	is_eol		(ifstream&);

//----------------------------------------------------------
// read characters until a whitespace character is read
// note: this function calls read_advance()
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: Cstring    (string of characters read)
Cstring	read_word	(ifstream&);

//----------------------------------------------------------
// read characters until a linefeed (10) character is read
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: Cstring    (string of characters read)
Cstring	read_line	(ifstream&);

//----------------------------------------------------------
// read characters until a tilde '~' character is read
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: Cstring    (string of characters read)
Cstring	read_tilde	(ifstream&);

//----------------------------------------------------------
// read characters until a left bracket '}' character is read
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: Cstring    (string of characters read)
Cstring	read_bracket	(ifstream&);

//----------------------------------------------------------
// read a number from the input filestream.  This converts
// ascii to a signed integer.
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: int        (number read)
int	read_number	(ifstream&);

//----------------------------------------------------------
// read a number from the input filestream.  If the number is
// negative, return 0.
// note: this function calls read_advance()
// INPUT:  ifstream&  (input file stream)
// OUTPUT: int        (number read)
int	read_num_pos	(ifstream&);

#endif

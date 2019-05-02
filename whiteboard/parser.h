/* parser.h: Header file for parser.c */

#ifndef _PARSER_H
#define _PARSER_H

#include "graphics.h"

#define BITMASK 0x00FF

void commandParser(Display*, Widget, char[], int, char, XColor, int);
unsigned short getbits(unsigned short, int, int);
unsigned short get16Bit(char, char);
void PackandSend(int, unsigned short, unsigned short, unsigned short, unsigned short);
void SendLine(XPoint * ,int);
void SendText(TextObject *);

#endif

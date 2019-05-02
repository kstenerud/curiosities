//------------------------------------------------------------------------------
//	VT100 RECEIVE EMULATION
//
// NAME:        vt100rx.h
//
// SOURCE FILE: vt100rx.cpp
//
// FUNCTIONS: Takes a stream of characters 1 char at a time and interfaces with
//            a WindowDisplay object to produce VT100 emulation
//
//
// DATE: October 30 / 1996
//
// DESIGNER:   Karl Stenerud
// PROGRAMMER: Karl Stenerud
//
// NOTES: This function requires that a WindowDisplay object be defined
//
//------------------------------------------------------------------------------

#ifndef _VT100RX_H
#define _VT100RX_H
#include "windisp.h"

//------------------------------------------------------------------------------
// Name:   VT100Rx
// Func:   Decode VT100 emulation codes and send them to a Window Display.
//         Values are sent in 1 character at a time.  The function maintains
//         its current state between calls.
// Inputs: windisp: A window to display to
//         value:   The value to be interpreted.
// Output: void

void VT100Rx(WindowDisplay* windisp, char value);
#endif
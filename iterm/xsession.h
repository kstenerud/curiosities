#ifndef X_SESSION_H
#define X_SESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <windowsx.h>
#include <process.h> //added
#include <string.h>
#include "xdatalink.h"

#define ASCII_SOH 0x01
#define ASCII_STX 0x02
#define ASCII_EOT 0x04
#define ASCII_ACK 0x06
#define ASCII_NAK 0x15
#define ASCII_CAN 0x18
#define ASCII_C   0x43
#define ASCII_SUB 0x1a

#define XRCV_MAX_ERRORS			20
#define XRCV_MAX_TIMEOUTS		10
#define XRCV_128SIZE				128
#define XRCV_1KSIZE				1024
#define XRCV_TIMEOUT				5000
#define XRCV_SUCCESS				0
#define XRCV_EOT					1
#define XRCV_ERR_BAD_SEQUENCE	2
#define XRCV_ERR_TIMEOUT		3
#define XRCV_ERR_DUPLICATE		4
#define XRCV_ERR_BAD_HEADER	5
#define XRCV_ERR_CORRUPT		6
#define XRCV_ERR_USERQUIT     7
#define updcrc(crc, data) ( (unsigned short)(crc << 8) ^ \
   crc16tab[ (unsigned char)((crc >> 8) ^ data )] )

//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=-=-=-=Structures
struct x_pkt {
   unsigned char soh;
   unsigned char pktnum;
   unsigned char pktcmp;
   unsigned char data[128];
   unsigned short chkval;
};
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=-=-=-=-==-==-=-=


//xreceive specific
int waitchar(char*, DWORD ms);
void purgeserial();
unsigned short calc_crc(unsigned char*, unsigned short);
unsigned char calc_cksum(unsigned char*, int);

//xsend specific
int x_packetize(HANDLE fp,unsigned char blocknum, x_pkt * packet);
int x_sendfile(HWND hwnd, HANDLE hCOMM);
HANDLE Getfilename();
BOOL CALLBACK GetFileDialogFunc(HWND hdwnd, UINT message, WPARAM wParam,
                                LPARAM lParam);

#endif

#ifndef XDATALINK_H
#define XDATALINK_H

//*****************************************************************
//*

//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-Program includes
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <windowsx.h>
#include <process.h> //added
#include <string.h>
#include <fcntl.h>
#include <io.h>
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-==-=-=-=

//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=Program defines
#define CAN   0x18
#define EOT   0x04
#define ACK   0x06
#define NAK   0x15
#define SOH   0x01
#define BOOL  int
#define BYTE  unsigned char
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-==-=-=-=


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=-=-=-=Prototypes
//xsend specific
DWORD WINAPI x_sendThread(LPVOID param);

//xreceive specific
int xmodem_block_receive(char*, int*, int, unsigned char, BOOL);
BOOL CALLBACK SendingDialogFunc(HWND hdwnd, UINT message, WPARAM wParam,
                                LPARAM lParam);
BOOL CALLBACK ReceivingDialogFunc(HWND hdwnd, UINT message, WPARAM wParam,
                                LPARAM lParam);
DWORD WINAPI x_receive (LPDWORD lpdwParam1);
int xmodem_file_receive(HANDLE hFile, HWND hDlg);
DWORD WINAPI X_Read (LPDWORD lpdwParam1);
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=-=-=-=-==-==-=-=


#endif

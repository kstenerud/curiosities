#ifndef _BAERG_H
#define _BAERG_H

#define B_PORT            0xdc97
#define B_OP_POINTS       0x00
#define B_OP_LINE         0x01
#define B_OP_RECT         0x02
#define B_OP_ELLIPSE      0x03
#define B_OP_TEXT         0x04
#define B_OP_INITIALIZE   0x05 
#define B_OP_FILL_RECT    0x06
#define B_OP_FILL_ELLIPSE 0x07
#define B_OP_DISCONNECT   0x08	
#define B_OP_ERASE        0x09
#define B_OP_CLEARSCREEN  0x0a
#define B_OP_RESTORE_REQ  0x0b	
#define B_OP_RESTORE_ACK  0x0c	
#define B_OP_RESTORE_NAK  0x0d	
#define B_OP_RESTORE_DATA 0x0e	
#define B_OP_RESTORE_END  0x0f	
#define B_OP_INIT_MULTI   0x10

#define B_TYPE_SINGLE     0x00
#define B_TYPE_MULTI      0x80

/*========================== Protocol Extensions ===============================*/

#define B_MULTI_MAX_CONNECT 5

#define B_MULTI_REQ_SERVER  0	/* request server*/
#define B_MULTI_I_AM_SERVER 1	/* <server_ID> <your_ID> <NUM> <clent_IP> ...*/
#define B_MULTI_CONNECT_REQ 2	/* <server_IP> <my_ID>*/
#define B_MULTI_CONNECT_ACK 3	/* <my_ID>*/
#define B_MULTI_CONNECT_NAK 4	/* <server_IP>*/
				/* 0 if full*/

#endif

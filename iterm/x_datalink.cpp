#include <windows.h>
#include <windowsx.h>
#include "xdatalink.h"
#include "xsession.h"
#include "assign4.h"
#include "resrcs.h"

//=-=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=Global Variables

//--------- external variables -----------
extern HINSTANCE   		hsaveinst;
extern DWORD       		sendTid1;        // send thread id no.
extern HANDLE				hComm;           // handle to communication port
extern BOOL     			bReading = TRUE; // read thread flag
extern HWND					hwnd;
extern HANDLE				hreadThread;     // handle to read thread
extern WindowDisplay*   windowdisplay;
extern long int 			total_blocks;    // total No of blocks to send
extern HANDLE           sendfilehandle;  // handle of file to send
volatile BOOL     		CRC;             // CRC state variable

//--------- file scope variables -----------
HANDLE   					hsenddlg;        // handle to Xmodem send dialog
HANDLE   					hsendThread;     // handle to Xmodem send thread
HANDLE                  hrecThread;      // handle to Xmodem receive thread
HANDLE						fh;              // handle to file for Xmode receive

//--------- required by Xmodem receive -----------
CircularBuffer          x_circ_buff;     // circular buffer for X_Read thread
BOOL                    x_receive_quit;  // flag to tell xmodem receive to quit
HANDLE                  hrecdlg;         // handle to the xmodem receive dialog
BOOL                    x_read_quit;     // flag to tell X_Read to quit
//------------------------------------------------

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-= x_sendthread
// DATALINK LAYER (SEND)
//------------------------------------------------------------------------------
// Name:       x_sendThread (thread)
// Programmer: Michael Gris
// Date:       06-Dec-96
// Desc:       The "main" xmodem send function.  Starts the read thread,
//             runs the xmodem send protocol, then cleans up.
// Inputs:     hComm (from lpdwParam1): handle to an open Comm port
// Return:     required for threads, otherwise useless.
// Notes:      This function comprises the DATALINK LAYER of xmodem send. A few
//					of the coding ideas were borrowed from Joe Campbell.
DWORD WINAPI x_sendThread(LPVOID lpdwParam)
{
   HANDLE*        params = (HANDLE*) lpdwParam;  // thread parameters array
   HANDLE         hComm = params[0];             // handle to open communication port
   HANDLE         hFile = params[1];             // handle to file to send
   DWORD				thread_id;                     // thread id
   HANDLE			x_read_thread;                 // handle to thread X-read
  	int            retries;                       // error retries
   int            i;                             // indexing variable
	int            file_status;                   // send file status, EOF or otherwise
   int            packet_size;                   // size of Xmodem send packet
   int				error_cnt = 0;                 // error count
   char				scratch[80];
   unsigned char  temp;
	unsigned char  inchar;								 // character received from receiver
   unsigned char	block_no = 1;                  // packet sequence number
   unsigned long  blocknum = 1;                  // ditto
   x_pkt          packetbuff;                  	 // buffer used to store packet
   BOOL				REC_NOT_ACK;                   // receiver acknowledge variable
   BOOL				EOF_STATE;                     // End of file state variable
   BOOL				X_SEND;                        // Xmodem send state variable
	BOOL           x_badpkt;                      // Bad packet state variable

   // set states
	X_SEND = TRUE;
   REC_NOT_ACK = TRUE;
   EOF_STATE = FALSE;

   // set retries
   retries = 1;

   // set state of read thread
   x_read_quit = FALSE;

   //start X_READ thread
	if ((x_read_thread=CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) X_Read,
     									 		hComm, 0, &thread_id)) == INVALID_HANDLE_VALUE)
	{
      MessageBox (NULL, "Error creating read thread, Aborting", "", MB_OK);
    	X_SEND = FALSE;
   }

   sprintf(scratch, "%d", total_blocks);
	SendDlgItemMessage(hsenddlg,IDC_XSEND_EDIT1,WM_SETTEXT,(WPARAM) 0,
     					    (LPARAM) (LPSTR) scratch);

   //wait for startup character, timeout after 60 sec
   while  ((X_SEND==TRUE) && (REC_NOT_ACK == TRUE))
   {
   	SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
     					       (LPARAM) (LPSTR) "Negotiating");

      if ((x_circ_buff.waitchar(&inchar, 10000)) == FALSE)
         inchar = 'T';

      switch (inchar)
      {
         // receiver wants CRC mode
         case 'C' :
            CRC = TRUE;
            packet_size = 133;
         	SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
         					       (LPARAM) (LPSTR) "CRC mode selected");
            REC_NOT_ACK = FALSE;
            break;

         // receiver wants Checksum mode
         case NAK :
         	SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
         					    	 (LPARAM) (LPSTR) "Checksum mode selected");
            packet_size = 132;
            REC_NOT_ACK = FALSE;
            break;

         // receiver cancel
         case CAN :
            X_SEND = FALSE;
         	SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
         					    (LPARAM) (LPSTR) "Receiver cancelled");
            REC_NOT_ACK = FALSE;
            SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
     					       (LPARAM) (LPSTR) "Transfer Cancelled");
            break;

         // timeout
         case 'T':
            retries++;
	         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
	   	     					    (LPARAM) (LPSTR) "Negotiation Timeout");
            if (retries >= 6)
            {
               X_SEND = FALSE;
               SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
     					    	 (LPARAM) (LPSTR) "Too many timeouts... Aborting");
            	writeByteToPort(CAN);
            }
            break;
      }
   }

   // set bad packet state
   x_badpkt = FALSE;

   // reset retries
   retries = 1;

   //transmission loop
   while (X_SEND && (retries < 10))
   {
      if (x_badpkt == FALSE)
      {
         if (!(EOF_STATE))
         	if ((file_status = x_packetize(hFile,block_no,&packetbuff)) > 0)
            {
               sprintf(scratch, "%u", blocknum);
               SendDlgItemMessage(hsenddlg,IDC_XSEND_EDIT2,WM_SETTEXT,(WPARAM) 0,
                               (LPARAM) (LPSTR) scratch);
               retries = 0;
               x_badpkt = FALSE;
					SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
	        						       (LPARAM) (LPSTR) "Sending data....");
            }
      }

      if (file_status == -1)
      {
         X_SEND = FALSE;
         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
         					    (LPARAM) (LPSTR) "Error reading file...Aborting");
         MessageBox (NULL, "File Read Error, Aborting", "", MB_OK);
         break;
      }

      if (!(EOF_STATE))
		{
         writeByteToPort(packetbuff.soh);                //send SOH
         writeByteToPort(packetbuff.pktnum);					//send block number
         writeByteToPort(packetbuff.pktcmp);             //send block no complement
         writeDataToPort(packetbuff.data, 128);

         if (CRC)
            writeByteToPort((unsigned char) (packetbuff.chkval >> 8));

         writeByteToPort((unsigned char) packetbuff.chkval);
       }

      if (file_status == 0)
      {
         EOF_STATE = TRUE;
         writeByteToPort(EOT);
         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
         					    (LPARAM) (LPSTR) "End of file reached");
      }

      // wait for some kind of ackknowledgement
      if (X_SEND==TRUE)
      {
         // try ten times
         for (i=1; i <= 10; i++)
         {
            //1 second timeouts
            x_circ_buff.waitchar(&inchar, 1000);

            if (inchar == ACK)
            {
               blocknum++;
            	block_no++;
               break;
            }

            // packet not ok, resend packet
            if (inchar == NAK)
            {
               if (EOF_STATE)
               {
                  writeByteToPort(EOT);
                	X_SEND = FALSE;
               }
               else
               {
                  x_badpkt = TRUE;
                  sprintf(scratch,"%d",++error_cnt);
                  SendDlgItemMessage(hsenddlg,IDC_XSEND_EDIT3,WM_SETTEXT,(WPARAM) 0,
                               (LPARAM) (LPSTR) scratch);
                  SendDlgItemMessage(hsenddlg,IDC_XSEND_EDIT4,WM_SETTEXT,(WPARAM) 0,
                               (LPARAM) (LPSTR) "Packet error");

                  retries++;

                  // timeout
                  if (retries >= 10)
                     X_SEND = FALSE;
					}
               break;
            }

            // receiver cancel
            if (inchar == CAN)
            {
               X_SEND = FALSE;
		         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
         					    (LPARAM) (LPSTR) "Exiting....");
               break;
            }

            if (i == 0)
            {
		         SendDlgItemMessage(hsenddlg,IDC_XSEND_EDIT4,WM_SETTEXT,(WPARAM) 0,
                               (LPARAM) (LPSTR) "Timeout");
               x_badpkt = TRUE;
            }
         }
   	}
   }

   // End of transmission phase
   for (i=0; i<10; i++)
   {
      if ((x_circ_buff.waitchar(&inchar, 1000)) == FALSE)
      {
         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
                            (LPARAM) (LPSTR) "Timeout");
         continue;
      }

      if (inchar == NAK)
      {
         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
                            (LPARAM) (LPSTR) "Timeout");
         break;
      }

      if (inchar == ACK)
      {
         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
                            (LPARAM) (LPSTR) "Transfer complete");
         writeByteToPort(EOT);
         break;
      }

      if (inchar == CAN)
      {
         SendDlgItemMessage(hsenddlg,IDC_XSEND_STATUSBAR,WM_SETTEXT,(WPARAM) 0,
                            (LPARAM) (LPSTR) "Receiver cancelled");
         break;
      }
   }

   CloseHandle(hFile);
   SendDlgItemMessage(hsenddlg,IDCANCEL,WM_SETTEXT,0, (LPARAM) "OK");
   x_read_quit = TRUE;
	return 1;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-= x_sendthread


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=receiveThread
// DATALINK LAYER (RECEIVE)
//------------------------------------------------------------------------------
// Name:       x_receive (thread)
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       The "main" xmodem receive function.  Starts the read thread,
//             runs the xmodem receive protocol, then cleans up.
// Inputs:     hFile (from lpdwParam1): handle to an open file for writing
//             hComm (from lpdwParam1): handle to an open Comm port
// Return:     required for threads, otherwise useless.
// Notes:      This function comprises the DATALINK LAYER of xmodem receive.
DWORD WINAPI x_receive (LPDWORD lpdwParam1)
{
   HANDLE* parms = (HANDLE*) lpdwParam1;   // array pointer for parameters
   HANDLE hFile   = parms[0];              // handle to open file (write)
   HANDLE hComm   = parms[1];              // handle to open comm port
   HWND hDlg      = hrecdlg;               // handle to open dialog box
   HANDLE x_read_thread;                   // handle for read thread
   DWORD  thread_id;                       // id for read thread

   // reset quit flag
   x_receive_quit = FALSE;
   x_read_quit = FALSE;

   // start X_READ thread
   x_read_thread=CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) X_Read,
                               hComm, 0, &thread_id);
   if(x_read_thread == INVALID_HANDLE_VALUE)
   {
      SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT,0,
     					       (LPARAM) "Couldn't start read thread!");
      SendDlgItemMessage(hDlg,IDCANCEL,WM_SETTEXT,0, (LPARAM) "OK");
      x_receive_quit = TRUE;
      return 0;
   }

   // start xmodem
   xmodem_file_receive(hFile, hDlg);

   // when xmodem ends, change "CANCEL" button to "OK"
   SendDlgItemMessage(hDlg,IDCANCEL,WM_SETTEXT,0, (LPARAM) "OK");
   x_receive_quit = TRUE;

   // kill X_READ thread
   x_read_quit = TRUE;

   // clear out the circular buffer
   x_circ_buff.clear();

   return 0;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-

//-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=xmodem_block_receive
// DATA LINK LAYER (RECEIVE)
//------------------------------------------------------------------------------
// Name:       xmodem_block_receive
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       Implements the xmodem protocol to receive 1 block of data.
// Inputs:     Buffer:    pointer to buffer to fill with data.
//             length:    stores length of data written to the buffer.
//             timeout:   timeout value in milliseconds.
//             block_num: current block we are looking for.
//             crc_mode:  TRUE = CRC, FALSE = Checksum mode.
// Return:     See datalink.h for list of return codes.
// Notes:      This function comprises the DATA LINK LAYER of xmodem receive.
int xmodem_block_receive(char* Buffer, int* length, int timeout,
                         unsigned char block_num, BOOL crc_mode)
{
   unsigned char  ch              = 0;
   unsigned char  block_rcv       = 0;
   unsigned char  block_rcv_compl = 0;
   unsigned char  cksum_value     = 0;
   unsigned short crc_value       = 0;
            int   i               = 0;

   if(x_receive_quit == TRUE)                       // User pressed "CANCEL"
      return XRCV_ERR_USERQUIT;

   if(!x_circ_buff.waitchar(&ch, timeout))          // get 1 char
		return XRCV_ERR_TIMEOUT;

	if(ch == ASCII_SOH)	                            // 128 byte packet
		*length = XRCV_128SIZE;
	else if(ch == ASCII_STX)	                      // 1024 byte packet
		*length = XRCV_1KSIZE;
	else if(ch == ASCII_EOT)	                      // End of transmission
		return XRCV_EOT;
	else                                             // invalid data
		return XRCV_ERR_BAD_HEADER;

	if(!x_circ_buff.waitchar(&block_rcv, timeout))   // block#
		return XRCV_ERR_TIMEOUT;
   if(block_rcv +1 == block_num)	                   // duplicate block
      return XRCV_ERR_DUPLICATE;
   else if(block_rcv != block_num)	                // out of sequence
      return XRCV_ERR_BAD_SEQUENCE;

	if(!x_circ_buff.waitchar(&block_rcv_compl, timeout)) // block# complement
      return XRCV_ERR_TIMEOUT;
   if( ((unsigned char)(~block_rcv_compl)) != block_num) // bad block#
      return XRCV_ERR_CORRUPT;

	for(i=0;i<*length;i++)                           // data portion
   {
      if( !x_circ_buff.waitchar((Buffer+i), timeout) )
         return XRCV_ERR_TIMEOUT;
   }

	if(!x_circ_buff.waitchar(&ch, timeout))	       // get 1 char
      return XRCV_ERR_TIMEOUT;

   if(crc_mode)
   {
      crc_value =  ((unsigned short) ch) << 8;	    // grab CRC

		if(!x_circ_buff.waitchar(&ch, timeout))       // get 1 char
			return XRCV_ERR_TIMEOUT;

      crc_value += (unsigned short) ch;	          // grab rest of CRC

		if(calc_crc(Buffer, *length) != crc_value)    // bad CRC
			return XRCV_ERR_CORRUPT;
	}
	else
		if(calc_cksum(Buffer, *length) != ch)         // bad checksum
			return XRCV_ERR_CORRUPT;

   return XRCV_SUCCESS;                             // got 1 block
}

//-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=



//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=- SendingDialogFunc
// MONITOR (RECEIVE)
//------------------------------------------------------------------------------
// Name:       SendingDialogFunc
// Programmer: Michael Gris
// Date:       06-Dec-96
// Desc:       Handles messages to the xmodem sending dialog box.
// Inputs:     hdwnd:   handle to the dialog box.
//             message: message to send to the dialog box.
//             wParam:  argument to the message.
//             lParam:  other argument to the message.
// Return:     TRUE if the function understood the message.
// Notes:      There's a hack in place from x_receive to change the CANCEL
//             button to an OK button when the xmodem send has finished.
//             I'm using x_receive_quit as a flag value to determine if
//             the CANCEL button is in "cancel" mode (stop the xmodem receive)
//             or in "ok" mode (close the dialog box)
BOOL CALLBACK SendingDialogFunc(HWND hdwnd, UINT message, WPARAM wParam,
                                LPARAM lParam)
{
   //save dialog handle;
   hsenddlg = hdwnd;

	switch (message) {
   	case WM_COMMAND:
      	switch (LOWORD(wParam)) {
         	case IDCANCEL :
               EndDialog(hdwnd,  0);
               CloseHandle(sendfilehandle);
               return 1;
         }
   	return 1;
   }
   return 0;
}
//-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=- ReceivingDialogFunc
// MONITOR (RECEIVE)
//------------------------------------------------------------------------------
// Name:       ReceivingDialogFunc
// Programmer: Karl Stenerud, Michael Gris
// Date:       06-Dec-96
// Desc:       Handles messages to the xmodem receive dialog box.
// Inputs:     hdwnd:   handle to the dialog box.
//             message: message to send to the dialog box.
//             wParam:  argument to the message.
//             lParam:  other argument to the message.
// Return:     TRUE if the function understood the message.
// Notes:      There's a hack in place from x_receive to change the CANCEL
//             button to an OK button when the xmodem send has finished.
//             I'm using x_receive_quit as a flag value to determine if
//             the CANCEL button is in "cancel" mode (stop the xmodem receive)
//             or in "ok" mode (close the dialog box)
BOOL CALLBACK ReceivingDialogFunc(HWND hdwnd, UINT message, WPARAM wParam,
                                LPARAM lParam)
{
   hrecdlg = hdwnd;  //save dialog handle so we can call this dialog

	switch (message) {
   	case WM_COMMAND:
      	switch (LOWORD(wParam)) {
         	case IDCANCEL :        // We only worry about the CANCEL button
               if(!x_receive_quit)
               {
                 // If xmodem hasn't quit, we are in "cancel" mode.
                 // This means we should abort the transfer.
                  SendDlgItemMessage(hdwnd,IDC_XREC_STATUSBAR,WM_SETTEXT, 0,
    					                   (LPARAM) "Aborting, please wait...");
                  x_receive_quit = TRUE;  // flags xmodem_block_receive to quit
                  return 1;
               }
               // otherwise we are in "ok" mode, so close the dialog.
               EndDialog(hdwnd,  0);
               return 1;
         }
   	return 1;
   }
   return 0;
}
//-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-= X-Read
// PHYSICAL LAYER (XMODEM SEND AND RECEIVE)
//------------------------------------------------------------------------------
// Name:       X_READ (thread)
// Programmer: Karl Stenerud, Rudi Pfenniger
// Date:       06-Dec-96
// Desc:       Takes data off the com port and places them in a circular buffer.
// Inputs:     hComm (from lpdwParam1): handle to an open com port.
// Return:     required for threads, otherwise useless.
// Notes:      This function comprises the PHYSICAL LAYER of xmodem.
DWORD WINAPI X_Read (LPDWORD lpdwParam1)
{
   static HANDLE  hComm;            // handle to comm port
   static DWORD   nBytesRead;       // how many bytes we read from the port
   static DWORD   dwEvent;          // indicates the event that occurred
   static DWORD   dwError;          // indicates an error that occurred
   static COMSTAT cs;               // info about the communications device
   static int     i;                // generic counter
   static char    buffer[2048];     // buffer to store bytes read from the port

   // get comm port
   hComm = (HANDLE) lpdwParam1;

   // clear the buffer before we begin
   x_circ_buff.clear();

	/* generate event whenever a byte arives */
	SetCommMask (hComm, EV_RXCHAR);

	/* read a byte whenever one is received */
	while(!x_read_quit)
   {
		/* wait for event */
		if (WaitCommEvent (hComm, &dwEvent, NULL))
		{
         if(x_read_quit)
            break;
			/* read all available bytes */
			ClearCommError (hComm, &dwError, &cs);
			if ((dwEvent & EV_RXCHAR) && cs.cbInQue)
			{
			  if (!ReadFile(hComm, buffer, cs.cbInQue,&nBytesRead, NULL))
				 locProcessorCommError(GetLastError());
			  else
			  {
             // fill the circular buffer
             for(i=0;i<nBytesRead;i++)
                x_circ_buff.put(buffer[i]);
			  }
			}
		}
		else
		  locProcessorCommError(GetLastError());
	}
	/* clean out any pending bytes in the receive buffer */
	PurgeComm(hComm, PURGE_RXCLEAR);
	return 0L;
} /* end function (ReadThread) */

//-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=

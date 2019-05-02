#include "circbuf.h"
#include "xsession.h"
#include "assign4.h"


//=-=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=Global Variables

//--------- External variables ------------------

//--------- Originate in Datalink.cpp
extern CircularBuffer   x_circ_buff;
extern HANDLE 				hsendThread;
extern HINSTANCE   		hsaveinst;
extern BOOL             x_receive_quit;
extern BOOL					CRC;

//--------- Originate in Iterm.cpp
extern HWND     			hwnd;
extern HANDLE   			hsenddlg;
//-----------------------------------------------

extern DWORD				id;

//--------- File scope variables ------------------
DWORD      					file_length = 0;
long int 					total_blocks = 0;
div_t      					div_struct;
int        					packetdatalen = 128;
HANDLE  						sendfilehandle;
//-------------------------------------------------


//--------- CRC table used by calc_crc() ------------------
// Author:  Anthon Pang
// Date:    ??/??/??
unsigned short crc16tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
//=-=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-= calc_crc
// SESSION LAYER
//------------------------------------------------------------------------------
// Name:       calc_crc
// Programmer: Anthon Pang
// Date:       ??-???-??
// Desc:       calculates CRC on an array of data.
// Inputs:     buf: pointer to data
//             len: length of data in bytes
// Return:     The CRC of the data
// Notes:      -
unsigned short calc_crc(unsigned char *buf, unsigned short len)
{
    unsigned short crc = 0;

	 while (len--)
		  crc = (unsigned short)(crc << 8)
            ^ crc16tab[ (unsigned char)( (crc >> 8) ^ *buf++ ) ];

    return crc;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-= calc_crc


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-= calc_cksum
//	SESSION LAYER
//------------------------------------------------------------------------------
// Name:       calc_cksum
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       calculates the checksum on an array of data.
// Inputs:     buffer: pointer to data
//             length: length of data in bytes
// Return:     The checksum of the data
// Notes:      -
unsigned char calc_cksum(unsigned char* buffer, int length)
{
	unsigned char cksum = 0;
				int  i;

   for(i=0;i<length;i++)
      cksum += buffer[i];

   return cksum;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-= calc_cksum


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=- x_packetize
//	SESSION LAYER
//------------------------------------------------------------------------------
// Name:       x_packetize
// Programmer: Nichael Gris
// Date:       05-Dec-96
// Desc:       Pieces together an XModem packet
//				   with a standard data field of 128 bytes
// Inputs:     fh: handle of file to send
//             blocknum: current block number
// Return:     The status of the packetize function
//             -1: file read error
//             1:  packet made correctly
//					0:  End of file reached
// Notes:-
int x_packetize(HANDLE fh,unsigned char blocknum, x_pkt *packet)
{
	int 	bytes_read;      // number of bytes read
	int 	bytes_left;      // number of bytes remaining
   BOOL  fileread;        // fileread flag, used for error checking

   packet->soh = SOH;                      // start of header
	packet->pktnum = blocknum % 256;        // block number
	packet->pktcmp = ~(packet->pktnum);     // complement of block number

   // read the data
   fileread = ReadFile(fh,packet->data,packetdatalen,&bytes_read,NULL);

   if (fileread == FALSE)
      return -1;
   else if (bytes_read < packetdatalen)   //EOF reached, data portion read is < 128 pad with zeros
	{
      bytes_left = packetdatalen-bytes_read;
      for (int i= bytes_read; i<=bytes_left; i++)
         packet->data[i] = 0x1A;
	}
   if (CRC)
		packet->chkval = calc_crc(packet->data, packetdatalen);
	else
		packet->chkval = ((calc_cksum(packet->data,packetdatalen) << 8));

	if (bytes_read == 0)
      return 0;
   else
   	return 1;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=- Getfilename
//	SESSION LAYER
//------------------------------------------------------------------------------
// Name:       Getfilename
// Programmer: Nichael Gris
// Date:       05-Dec-96
// Desc:       Retrieves a file handle for a file to send.
// Inputs:     None.
// Return:     The Handle to a file
// Notes:-     There is no error checking for invalid file handles
//					here
HANDLE Getfilename(void)
{
   DWORD    error;
   char fileName[_MAX_PATH];
   char titleName[_MAX_FNAME + _MAX_EXT];
   OPENFILENAME ofn;
   HANDLE  fileh;

   fileName[0] = titleName[0] = 0;

   // structure required for open file requestor dialog
   ofn.lStructSize       = sizeof (OPENFILENAME);
   ofn.hwndOwner         = hwnd;
   ofn.hInstance         = NULL;
   ofn.lpstrFilter       = "All Files (*.*)\0*.*\0\0";
   ofn.lpstrCustomFilter = NULL;
   ofn.nMaxCustFilter    = 0;
   ofn.nFilterIndex      = 0;
   ofn.lpstrFile         = fileName;
   ofn.nMaxFile          = _MAX_PATH;
   ofn.lpstrFileTitle    = titleName;
   ofn.nMaxFileTitle     = _MAX_FNAME + _MAX_EXT;
   ofn.lpstrInitialDir   = NULL;
   ofn.lpstrTitle        = NULL;
   ofn.Flags             = 0;
   ofn.nFileOffset       = 0;
   ofn.nFileExtension    = 0;
   ofn.lpstrDefExt       = NULL;
   ofn.lCustData         = 0L;
   ofn.lpfnHook          = NULL;
   ofn.lpTemplateName    = NULL;

   if(!GetOpenFileName(&ofn))   // Open file requestor
		MessageBox(hwnd, "Bad File Name", "", MB_OK);

   // open the file
   fileh = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   return fileh;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-= x_sendfile
//	SESSION LAYER
//------------------------------------------------------------------------------
// Name:       x_sendfile
// Programmer: Nichael Gris
// Date:       05-Dec-96
// Desc:       Opens a file handle, checks it for validity,
//             if valid, calculates filelength, number of
//					XModem 128 byte blocks to be sent, and starts
//					the main file send thread.
// Inputs:     hwnd: handle to main application.
//					hComm: handle to open communications port.
// Return:     some integer value which is quite useless.
// Notes:-
int x_sendfile(HWND hwnd, HANDLE hCOMM)
{
   HANDLE  				sparms[2];
   HANDLE  				x_send_thread;
   DWORD   				sendTid1;           //send thread id no.

   sendfilehandle = Getfilename();

   if (sendfilehandle == INVALID_HANDLE_VALUE)
   {
      MessageBox (NULL, "Invalid file handle", "", MB_OK);
      return 0;
   }
   else
   {
      file_length = GetFileSize(sendfilehandle,NULL);
      if (file_length <= 0)
         MessageBox (NULL, "Invalid file length", "", MB_OK);
      else
      {
         div_struct = div(file_length,packetdatalen);
         total_blocks = div_struct.quot;
         if (div_struct.rem)                   	// if true there is a block less then 128 bytes
            total_blocks++;

         sparms[0] = hCOMM;
         sparms[1] = sendfilehandle;

         if ((x_send_thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)x_sendThread,
                                           sparms,0,&sendTid1)) == INVALID_HANDLE_VALUE)
         {
            MessageBox (NULL, "Unable to create send thread", "", MB_OK);
            return 0;
         }

   	   DialogBox(hsaveinst,"IDD_XSEND_DIALOG",hwnd, SendingDialogFunc);
 			// DialogBox returns when user presses "OK" or "CANCEL"
   	}
   }

   TerminateThread(x_send_thread, 0);

	return 1;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-


//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=xmodem_file_receive
// SESSION LAYER (RECEIVE)
//------------------------------------------------------------------------------
// Name:       xmodem_file_receive
// Programmer: Karl Stenerud
// Date:       06-Dec-96
// Desc:       Implements the xmodem protocol to receive a file.
// Inputs:     hFile: handle to an open file for writing
//             hDlg: handle to an open dialog
// Return:     TRUE if transfer was successful, otherwise FALSE
// Notes:      This function comprises the SESSION LAYER of xmodem receive.
int xmodem_file_receive(HANDLE hFile, HWND hDlg)
{
   char          buf1[XRCV_1KSIZE];    // buffer to hold xmodem blocks
   char          buf2[XRCV_1KSIZE];    // second buffer to hold xmodem blocks
   char*         last_read = buf1;     // pointer for "last read" buffer
   char*         this_read = buf2;     // pointer for "this read" buffer
   char*         temp_read = NULL;     // temp pointer for pointer swapping
   int           last_length = 0;      // length of "last read"
   int           this_length = 0;      // length of "this read"
   char          SScratch[80];         // scratch buffer
   char          scratch;              // scratch char
   int           i;                    // counter
   unsigned char block_num = 1;        // block sequence number
	int           written;              // bytes written count for file writes
   int           total_written = 0;    // total bytes written to file
	int           errors = 0;           // total errors
	int           timeouts = 0;         // timeouts
	int           result = 0;           // result of xmodem_block_receive
	BOOL          crc_mode = TRUE;      // mode: TRUE = CRC, FALSE = Checksum
	int           blocks_read = 0;      // total blocks read
   BOOL          eot = FALSE;          // end of transmission status
   BOOL          err = FALSE;          // error status
   BOOL          t_o = FALSE;          // timeout status
   BOOL          can = FALSE;          // cancel status
   BOOL          dup = FALSE;          // dupolicate packet status
   char* xrcv_msg[] = {"Read 1 block",
                       "End of Transmission",
                       "Out of Sequence",
                       "Timeout",
                       "Duplicate Packet",
                       "Bad Header",
                       "Corrupt Packet",
                       "Aborted."
                      };               // messages for dialog box


   x_circ_buff.clear();                // clear buffer

   SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT,0,
     					    (LPARAM) "Initiating CRC Transfer");

   do					// try CRC, 5 retries
	{
		writeByteToPort(ASCII_C);
		result = xmodem_block_receive(this_read, &this_length, XRCV_TIMEOUT,
                                    block_num, crc_mode);
      if(result > 0)
         SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT,0,
      					       (LPARAM) xrcv_msg[result]);
	} while (result == XRCV_ERR_TIMEOUT && timeouts++ < 5);

	if(result == XRCV_SUCCESS)			         // successful read of 1 block
   {
      block_num++;
      last_length = this_length;             // save length
      temp_read = last_read;                 // swap buffers
      last_read = this_read;
      this_read = temp_read;

      sprintf(SScratch, "%d", ++blocks_read);
      SendDlgItemMessage(hDlg,IDC_XREC_EDIT1,WM_SETTEXT,0,(LPARAM) SScratch);

      writeByteToPort(ASCII_ACK);
   }
	else	if(result == XRCV_ERR_TIMEOUT)		// CRC not supported. Use checksum
   {
      SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT,0,
      					    (LPARAM) "Switching to Checksum");
      errors = timeouts = 0;
		crc_mode = FALSE;
      writeByteToPort(ASCII_NAK);
	}
	else
   {
		writeByteToPort(ASCII_NAK);			   // some other error...
      errors++;
      timeouts = 0;
   }

	for(;;)                                   // main loop
	{
		result = xmodem_block_receive(this_read, &this_length, XRCV_TIMEOUT,
                                    block_num, crc_mode);
      // report result in dialog box.
      if(result > 0)                         // Just report errors
         SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT,0,
      					       (LPARAM) xrcv_msg[result]);
		switch(result)
		{
		case XRCV_SUCCESS:
			if(temp_read != NULL)		         // only write if >1 buffers read
            // write previous buffer
				WriteFile(hFile, last_read, last_length, &written, NULL);
         last_length = this_length;          // save length
         temp_read = last_read;              // swap buffers
         last_read = this_read;
         this_read = temp_read;
			timeouts = 0;
         block_num++;
         sprintf(SScratch, "%d", ++blocks_read);
         SendDlgItemMessage(hDlg,IDC_XREC_EDIT1,WM_SETTEXT,0,
                            (LPARAM) SScratch);
         total_written += written;
         sprintf(SScratch, "%d", total_written);
         SendDlgItemMessage(hDlg,IDC_XREC_EDIT3,WM_SETTEXT,0,
                            (LPARAM) SScratch);
			writeByteToPort(ASCII_ACK);
			break;
		case XRCV_EOT:
         if(!eot)                            // first EOT?
         {
            eot = TRUE;
            writeByteToPort(ASCII_NAK);      // send NAK to be sure
         }
         else                                // second EOT
         {
            // strip off the SUBs
            for(i=last_length;(i>0) && (last_read[i-1] == ASCII_SUB);i--)
			   	;
            if(i>0)
               // Write final buffer
			      WriteFile(hFile, last_read, i, &written, NULL);
            total_written += written;
            sprintf(SScratch, "%d", total_written);
            SendDlgItemMessage(hDlg,IDC_XREC_EDIT3,WM_SETTEXT, 0,
                               (LPARAM) SScratch);
            SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT, 0,
      					          (LPARAM) "Completed.");
            writeByteToPort(ASCII_ACK);
			   return TRUE;
         }
         break;
		case XRCV_ERR_BAD_SEQUENCE:
         err = TRUE;
         break;
		case XRCV_ERR_TIMEOUT:
         t_o = TRUE;
			break;
		case XRCV_ERR_DUPLICATE:
         dup = TRUE;
			break;
		case XRCV_ERR_BAD_HEADER:
         err = TRUE;
			break;
		case XRCV_ERR_CORRUPT:
         err = TRUE;
			break;
		case XRCV_ERR_USERQUIT:
         SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT, 0,
    					          (LPARAM) "Aborted.");
         can = TRUE;
         break;
		default:
         can = TRUE;
			break;
		}
      if(t_o)                               // Timeout
      {
         if(++timeouts > XRCV_MAX_TIMEOUTS)
            can = TRUE;
         else
			   writeByteToPort(ASCII_NAK);
      }
      if(err)                               // Error
      {
         if(++errors > XRCV_MAX_ERRORS)
         {
            SendDlgItemMessage(hDlg,IDC_XREC_STATUSBAR,WM_SETTEXT,0,
      					          (LPARAM) "Too many errors! Aborting.");
            can = TRUE;
         }
         else
         {
            sprintf(SScratch, "%d", errors);
            SendDlgItemMessage(hDlg,IDC_XREC_EDIT2,WM_SETTEXT,0,
                               (LPARAM) SScratch);
            Sleep(1000);                    // wait to catch any more rogue data
            x_circ_buff.clear();            // then clear the buffer
	         writeByteToPort(ASCII_NAK);
         }
      }
      if(dup)                               // Duplicate packet
      {
         if(++errors > XRCV_MAX_ERRORS)
            can = TRUE;
         else
			   writeByteToPort(ASCII_ACK);
      }
      if(can)                               // Cancel transfer
      {
	      writeByteToPort(ASCII_CAN);        // make sure they get the message
	      writeByteToPort(ASCII_CAN);
         return FALSE;
      }
      err = t_o = dup = FALSE;
	}

   return FALSE;
}
//=-=-=-=-=-=--===-=-=-=-=-=-==-=-=-==-=-=-==-==-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-


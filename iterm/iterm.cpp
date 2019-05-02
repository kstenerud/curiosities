#include "windisp.h"
#include "assign4.h"
#include "circbuf.h"
#include "xsession.h"
#include "xdatalink.h"
#include "emu_import.h"

HDC      				hdc;
HWND     				hwnd;
HINSTANCE   			hsaveinst;
HANDLE   				hComm;
LPCSTR   				lpszCommName = "Com1";
DCB 						dcb;
COMMPROP 				cp;
COMMCONFIG 				cc;
int 						emulation = 2;
BOOL 						echo = FALSE;

HANDLE					hreadThread;  //Thread handle
DWORD						id;		//Thread id no


char 						szWinName[] = "Emu Communications";
char 						str[80] = "";

BOOL     				QUIT_PROG;
BOOL     				bReading = TRUE;

WindowDisplay* 		windowdisplay;


LRESULT CALLBACK WindowFunc(HWND, UINT, WPARAM, LPARAM);


int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,
						  LPSTR lpszArgs, int nWinMode)
{
	MSG msg;
	WNDCLASS wcl;

   wcl.hInstance = hThisInst;
   wcl.lpszClassName = szWinName;
	wcl.lpfnWndProc = WindowFunc;
   wcl.style = 0;
   wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
   wcl.lpszMenuName = "Assign4";
   wcl.cbClsExtra = 0;
   wcl.cbWndExtra = 0;
   wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

	if(!RegisterClass (&wcl)) return 0;

   hwnd = CreateWindow(szWinName,
							  "Emu Communications",
                       WS_OVERLAPPED | WS_CAPTION |
                       WS_SYSMENU | WS_MINIMIZEBOX |
                       WS_TILEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       ((SCREENX+1)*FONTX),
                       (((SCREENY+2)*FONTY)+5),
                       HWND_DESKTOP,
                       NULL,
                       hThisInst,
                       NULL
                       );

  windowdisplay = new WindowDisplay(hwnd, FONTX, FONTY, SCREENX, SCREENY);

  ShowWindow(hwnd, nWinMode);
  UpdateWindow(hwnd);

  hsaveinst = hThisInst;


  DialogBox(hsaveinst, "port", hwnd, PortDialogFunc);
            
  if ((hComm=CreateFile(lpszCommName, GENERIC_READ | GENERIC_WRITE, 0,
							  NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
  {
    MessageBox(NULL, "Error opening COM port", "", MB_OK);
    return FALSE;
  }

  cp.wPacketLength = sizeof(COMMPROP);
  GetCommProperties(hComm, &cp);

  bReading = TRUE;
  if ((hreadThread=CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) EmuRead,
			NULL, 0, &id)) == INVALID_HANDLE_VALUE)
  {
	 MessageBox (NULL, "Error creating READ thread", "", MB_OK);
	 return FALSE;
  }

	while(GetMessage(&msg, NULL, 0, 0))
	{
      TranslateMessage(&msg);
      DispatchMessage(&msg);
	}

  bReading = FALSE;
  ResumeThread(hreadThread);
  SetCommMask(hComm, 0L);
  while (GetExitCodeThread(hreadThread, &id))
  {
	 if (id == STILL_ACTIVE)
		continue;
	 else
		break;
  }

  CloseHandle(hreadThread);

  CloseHandle(hComm);    //closes the com port

	return msg.wParam;
}

LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
	unsigned long lrc;
	HMENU hMenu;

	hMenu = GetMenu(hwnd);

   switch(message)
	{
		case WM_KEYDOWN:
         switch(wParam)
         {
            // ENTER handled by ASCII
            // DELETE handled by ASCII
            // ^G (RESET) handled by ASCII
            case VK_UP:
					writeStringToPort("\x1bOA");
               break;
            case VK_DOWN:
               writeStringToPort("\x1bOB");
               break;
            case VK_BACK: // BACKSPACE
            case VK_LEFT:
               writeStringToPort("\x1bOD");
               break;
            case VK_RIGHT:
               writeStringToPort("\x1bOC");
               break;
            case VK_INSERT: // toggle insert mode
               writeStringToPort("\x1bOn");
               break;
            case VK_HOME: // clear screen
               writeStringToPort("\x1bOM");
               break;
				case VK_END: // PA1 -- find a key to use here.
            				 // watch out.. keys that send ascii will
                         // also generate a WM_CHAR message!
               writeStringToPort("\x1bOS");
               break;
				case VK_DELETE: // PA2 -- find a key to use here.
               writeStringToPort("\x1bOm");
               break;
				case VK_SNAPSHOT: // PA3 -- find a key to use here.
               writeStringToPort("\x1bOl");
               break;
				case VK_F1:
               writeStringToPort("\x1bOP");
               break;
            case VK_F2:
               writeStringToPort("\x1bOQ");
               break;
            case VK_F3:
               writeStringToPort("\x1bOR");
               break;
            case VK_F4:
               writeStringToPort("\x1bOw");
               break;
            case VK_F5:
               writeStringToPort("\x1bOx");
               break;
            case VK_F6:
               writeStringToPort("\x1bOy");
               break;
            case VK_F7:
               writeStringToPort("\x1bOt");
               break;
            case VK_F8:
               writeStringToPort("\x1bOu");
               break;
            case VK_F9:
               writeStringToPort("\x1bOv");
               break;
            case VK_F10:
               writeStringToPort("\x1bOq");
               break;
            case VK_F11:
               writeStringToPort("\x1bOr");
               break;
            case VK_F12:
               writeStringToPort("\x1bOs");
               break;
			}
			break;

		case WM_COMMAND:
		switch (LOWORD (wParam))
		{
       case IDM_NEW:
			 windowdisplay->ClearScreen(2);
			 windowdisplay->CursorXY(0,0);
			 break;

		  case IDM_PREFERENCES:

			 cc.dwSize = sizeof(COMMCONFIG);
			 cc.wVersion = 0x100;
			 GetCommConfig (hComm, &cc, &cc.dwSize);

			 if (!CommConfigDialog(lpszCommName,hwnd, &cc))
				break;
			 if (!SetCommState (hComm, &cc.dcb))
				break;
			 break;

			case IDM_EXIT:
			  QUIT_PROG = TRUE;
			  PostQuitMessage(0);
			  break;

			case IDM_NONE:
			 emulation = 0;
			 CheckMenuItem(hMenu, IDM_NONE, MF_CHECKED);
			 CheckMenuItem(hMenu, IDM_TVI950, MF_UNCHECKED);
			 CheckMenuItem(hMenu, IDM_VT100, MF_UNCHECKED);
			 break;

		  case IDM_TVI950:
			 emulation = 1;
			 CheckMenuItem(hMenu, IDM_NONE, MF_UNCHECKED);
			 CheckMenuItem(hMenu, IDM_TVI950, MF_CHECKED);
			 CheckMenuItem(hMenu, IDM_VT100, MF_UNCHECKED);
			 break;

		  case IDM_VT100:
			 emulation = 2;
			 CheckMenuItem(hMenu, IDM_NONE, MF_UNCHECKED);
			 CheckMenuItem(hMenu, IDM_TVI950, MF_UNCHECKED);
			 CheckMenuItem(hMenu, IDM_VT100, MF_CHECKED);
			 break;

		  case IDM_DIAL:
			 EscapeCommFunction(hComm, SETDTR);
			 EscapeCommFunction(hComm, SETRTS);
          EnableMenuItem(hMenu, IDM_DIAL, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_CONNECT, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_DISCONNECT, MF_ENABLED);
          DialogBox(hsaveinst, "dial", hwnd, DialDialogFunc);
			 //WriteFile(hComm, "atd 4351443\x0D", 13, &lrc, NULL);
		  break;

		  case IDM_CONNECT:
			 EscapeCommFunction(hComm, SETDTR);
			 EscapeCommFunction(hComm, SETRTS);
          EnableMenuItem(hMenu, IDM_DIAL, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_CONNECT, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_DISCONNECT, MF_ENABLED);
			 break;

		  case IDM_DISCONNECT:
			 EscapeCommFunction(hComm, CLRDTR);
			 MessageBox(hwnd, "Disconnected", "", MB_OK);
          EnableMenuItem(hMenu, IDM_DIAL, MF_ENABLED);
          EnableMenuItem(hMenu, IDM_CONNECT, MF_ENABLED);
          EnableMenuItem(hMenu, IDM_DISCONNECT, MF_GRAYED);
			 EscapeCommFunction(hComm, SETDTR);
			 break;

		  case IDM_ECHOON:
			 echo = TRUE;
			 CheckMenuItem(hMenu, IDM_ECHOON, MF_CHECKED);
			 CheckMenuItem(hMenu, IDM_ECHOOFF, MF_UNCHECKED);
			 break;

		  case IDM_ECHOOFF:
			 echo = FALSE;
			 CheckMenuItem(hMenu, IDM_ECHOON, MF_UNCHECKED);
			 CheckMenuItem(hMenu, IDM_ECHOOFF, MF_CHECKED);
			 break;

		  case IDM_SENDFILE:
           char		scratch[80];

           //terminate terminal program read thread
           SuspendThread(hreadThread);

           if ((x_sendfile(hwnd,hComm)) == 0)
	           MessageBox (NULL, "Unable to send", "", MB_OK);

           ResumeThread(hreadThread);

			  break;

		  case IDM_RECEIVEFILE:
            HANDLE Rparams[2];         // parameters to send to x_receive thread
            DWORD thread_id;           // ID for x_receive thread
            HANDLE x_rcv_thread;       // handle to x_receive thread
            char fileName[_MAX_PATH];              // used for file requestor
            char titleName[_MAX_FNAME + _MAX_EXT]; // used for file requestor
            HANDLE hSaveFile;                      // handle to file to save
            OPENFILENAME ofn;                      // used for file requestor

            fileName[0] = titleName[0] = 0;      // make sure no garbage in name

            // fill out the OPENFILENAME structure for the file requestor
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
            ofn.Flags             = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
            ofn.nFileOffset       = 0;
            ofn.nFileExtension    = 0;
            ofn.lpstrDefExt       = NULL;
            ofn.lCustData         = 0L;
            ofn.lpfnHook          = NULL;
            ofn.lpTemplateName    = NULL;

            if(!GetSaveFileName(&ofn))   // Open file requestor
               break;                    // user cancelled

            // open the file
            hSaveFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if(hSaveFile == INVALID_HANDLE_VALUE)
            {
               MessageBox(hwnd, "Error opening file", "", MB_OK);
               break;
            }

            // set up parameters to send to x_receive thread
            Rparams[0] = hSaveFile;
            Rparams[1] = hComm;

            // Suspend the terminal read thread
            SuspendThread(hreadThread);

            // Start the xmodem receive thread
            x_rcv_thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)
                                         x_receive, Rparams, 0, &thread_id);
            if(x_rcv_thread == INVALID_HANDLE_VALUE)
            {
               MessageBox (NULL, "Error creating Receive thread", "", MB_OK);
               break;
            }

            // Open the status dialog box
            DialogBox(hsaveinst,"IDD_XREC_DIALOG",hwnd, ReceivingDialogFunc);

            // DialogBox returns when user presses "OK" or "CANCEL"

            // close the file
            CloseHandle(hSaveFile);

            // Start the terminal thread again
            ResumeThread(hreadThread);

			 break;

		  case IDM_HELP:
          DialogBox(hsaveinst, "help1", hwnd, AboutDialogFunc);
          break;

        case IDM_HELP2:
          DialogBox(hsaveinst, "help2", hwnd, AboutDialogFunc);
          break;

        case IDM_HELP3:
          DialogBox(hsaveinst, "help3", hwnd, AboutDialogFunc);
          break;

		  case IDM_ABOUT:
			 DialogBox(hsaveinst, "about", hwnd, AboutDialogFunc);
		    break;
		  }
		  break;

		case WM_CHAR:
			if (echo == 1)
			  windowdisplay->WriteChar((char)wParam);
			if (!WriteFile(hComm,(LPBYTE)&wParam, 1, &lrc, NULL))
			  locProcessorCommError(GetLastError());
			break;

		case WM_PAINT:
         windowdisplay->Repaint();
         break;
      case WM_DESTROY:
         delete windowdisplay;
         PostQuitMessage(0);
         break;
      default:
         return DefWindowProc(hwnd, message, wParam, lParam);
   }
   return 0;
}

/****************************************************************************
FUNCTION:		DialDialogFunc

PROGRAMMER:		Rudi Pfenniger

INTERFACE: BOOL CALLBACK DialDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
INPUTS:    		HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam
OUTPUTS:

NOTES:

*****************************************************************************/

BOOL CALLBACK DialDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  char str[20];
  char dialstr[12];
  unsigned long lrc;

    switch(message)
    {
      case WM_COMMAND:
        switch(LOWORD(wParam))
        {
         case IDCANCEL:
           EndDialog(hdwnd, 0);
           return 1;
         break;

         case ID_DIAL:
           GetDlgItemText(hdwnd, ID_EB1, str, 20);
           str[19] = 0;
           //dialog string into a proper dialing string
           int j = 0;
           for (int i=0;i<strlen(str);i++)
              if isdigit(str[i])
              {
                 dialstr[j] = str[i];
                 j++;
              }
           dialstr[++j] = '\0';

           writeStringToPort("atdt ");
           writeStringToPort(dialstr);
           writeByteToPort((char) 0x0d);

           EndDialog(hdwnd, 0);
           return 1;
         break;
      }
  }
  return 0;
}

/****************************************************************************
FUNCTION:		PortDialogFunc

PROGRAMMER:		Rudi Pfenniger

INTERFACE: BOOL CALLBACK PortDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
INPUTS:    		HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam
OUTPUTS:

NOTES:

*****************************************************************************/

BOOL CALLBACK PortDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  char str[20];
  long i;
  unsigned long lrc;

    switch(message)
    {
      case WM_COMMAND:
        switch(LOWORD(wParam))
        {
         case IDCANCEL:
           lpszCommName = "Com1";
           EndDialog(hdwnd, 0);
           return 1;
			break;

			case ID_CONTINUE:
            i = SendDlgItemMessage(hdwnd, ID_LB1, LB_GETCURSEL, 0, 0L);
				switch (i)
				{
				  case 0:
                lpszCommName = "Com1";
				  break;

				  case 1:
                lpszCommName = "Com2";
				  break;

              case 2:
                lpszCommName = "Com3";
              break;

              case 3:
                lpszCommName = "Com4";
              break;

              default:
                lpszCommName = "Com1";
              break;
				}
				EndDialog(hdwnd, 0);
				return 1;
			break;

         case ID_LB1:
           if(HIWORD(wParam)==LBN_DBLCLK)
			  {
				 i = SendDlgItemMessage(hdwnd, ID_LB1, LB_GETCURSEL, 0, 0L);
				 switch (i)
				 {
					case 0:
					  sprintf(str, "Selected: Com 1");
                 lpszCommName = "Com1";
					break;

					case 1:
					  sprintf(str, "Selected: Com 2");
                 lpszCommName = "Com2";
					break;

               case 2:
					  sprintf(str, "Selected: Com 3");
                 lpszCommName = "Com3";
					break;

               case 4:
					  sprintf(str, "Selected: Com 4");
                 lpszCommName = "Com4";
					break;

               default:
                 sprintf(str, "Default setting: Com1");
                 lpszCommName = "Com1";
               break;
				 }
				 MessageBox(hdwnd, str, "Selection Made", MB_OK);
           }
         break;
      }
      break;
      case WM_INITDIALOG:
        SendDlgItemMessage(hdwnd, ID_LB1, LB_ADDSTRING, 0, (LPARAM)"Com 1");
        SendDlgItemMessage(hdwnd, ID_LB1, LB_ADDSTRING, 0, (LPARAM)"Com 2");
        SendDlgItemMessage(hdwnd, ID_LB1, LB_ADDSTRING, 0, (LPARAM)"Com 3");
        SendDlgItemMessage(hdwnd, ID_LB1, LB_ADDSTRING, 0, (LPARAM)"Com 4");
        return 1;
  }
  return 0;
}

/****************************************************************************
FUNCTION:		AboutDialogFunc

PROGRAMMER:		Rudi Pfenniger

INTERFACE: BOOL CALLBACK AboutDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
INPUTS:    		HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam
OUTPUTS:

NOTES:

*****************************************************************************/

BOOL CALLBACK AboutDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  char str[15];
  long i;
  unsigned long lrc;

    switch(message)
    {
      case WM_COMMAND:
        switch(LOWORD(wParam))
        {
         case IDCANCEL:
           EndDialog(hdwnd, 0);
           return 1;
			break;

			}
			break;
      }
  return 0;
}

/****************************************************************************
FUNCTION:		EmuRead	(for read thread)

PROGRAMMER:		Rudi Pfenniger

INTERFACE:    DWORD EmuRead (LPDWORD lpdwParam1)
INPUTS:					LPDWORD lpdwParam1
OUTPUTS:					DWORD

NOTES:

*****************************************************************************/
DWORD WINAPI EmuRead (LPDWORD lpdwParam1)
{
DWORD nBytesRead, dwEvent, dwError;
COMSTAT cs;
int i;
char temp;
char buffer[1024];
CircularBuffer emu_read_buff(2048);

	/* generate event whenever a byte arives */
	SetCommMask (hComm, EV_RXCHAR);

	/* read a byte whenever one is received */
	while (bReading) {
		/* wait for event */
		if (WaitCommEvent (hComm, &dwEvent, NULL))
		{
			/* read all available bytes */
			ClearCommError (hComm, &dwError, &cs);
			if ((dwEvent & EV_RXCHAR) && cs.cbInQue)
			{
			  if (!ReadFile(hComm, buffer, cs.cbInQue,&nBytesRead, NULL))
				 locProcessorCommError(GetLastError());
			  else
			  {
             for(i=0;i<nBytesRead;i++)
                if( emu_read_buff.put(buffer[i]) == -1)
                  MessageBox(hwnd, "Buffer full!", "", MB_OK);

				 if ((nBytesRead) && (emulation == 0))
				 {
                for(i=0;i<nBytesRead;i++)
                {
                  if(emu_read_buff.get(&temp) == -1)
                    MessageBox(hwnd, "Buffer empty!", "", MB_OK);
					   windowdisplay->WriteChar(temp);
                }
				 }
				 if ((nBytesRead) && (emulation == 2))
				 {
                for(i=0;i<nBytesRead;i++)
                {
                  if(emu_read_buff.get(&temp) == -1)
                    MessageBox(hwnd, "Buffer empty!", "", MB_OK);
		 			   VT100Rx(windowdisplay, temp);
                }
				 }
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

/****************************************************************************
FUNCTION:		locProcessorCommError

PROGRAMMER:		Rudi Pfenniger

INTERFACE:    void locProcessorCommError(DWORD dwError)
INPUTS:					DWORD dwError
OUTPUTS:					none

NOTES:

*****************************************************************************/
void locProcessorCommError(DWORD dwError)
{
  DWORD lrc;
  COMSTAT cs;

  ClearCommError(hwnd, &lrc, &cs);
}

int writeStringToPort(char* str)
{
   unsigned long temp;

	if (!WriteFile(hComm, str, strlen(str), &temp, NULL))
			  locProcessorCommError(GetLastError());

	return temp;
}

int writeByteToPort(char ch)
{
   unsigned long temp;

	if (!WriteFile(hComm, &ch, 1, &temp, NULL))
			  locProcessorCommError(GetLastError());

	return temp;
}

int writeDataToPort(char* str, int len)
{
   unsigned long temp;

	if (!WriteFile(hComm, str, len, &temp, NULL))
			  locProcessorCommError(GetLastError());

	return temp;
}


